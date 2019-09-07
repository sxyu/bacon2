#include "session.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include "util.hpp"
#include "core.hpp"

namespace {
#ifdef _WIN32
// Root directory for persistence (Windows)
const char * APPDATA = std::getenv("APPDATA");
const std::string STORAGE_ROOT = std::string(APPDATA) + "\\Bacon2\\";
#else
// Root directory for persistence (Unix-Like)
const char * HOME = std::getenv("HOME");
const std::string STORAGE_ROOT = std::string(HOME) + "/.bacon2/";
#endif
}  // namespace

namespace bacon {
// Session implementation
Session::Session(const std::string &name) : name(name) {
    if (!name.empty()) {
        util::create_dir(STORAGE_ROOT);
        std::string session_dir = STORAGE_ROOT + name;
        util::create_dir(session_dir);
        strats_path = session_dir + "/strategies";
        results_path = session_dir + "/results";
        config_path = session_dir + "/config";
        if (!load_state()) {
            throw std::runtime_error(std::string("Bacon internal error: failed to load persistent state for session: ") + name);
        }
    }
}

std::vector<std::string> Session::list_sessions() {
    return util::lsdir(STORAGE_ROOT);
}

Strategy::Ptr Session::add(Strategy::Ptr strategy) {
    if (strategy->sess != nullptr) {
        throw std::runtime_error("Adding strategy to multiple sessions is disallowed. Please remove it first.");
    }
    auto it = strategies.find(strategy->unique_id);
    if (it != strategies.end()) {
        it->second->sess = nullptr;
    }
    strategies[strategy->unique_id] = strategy;
    strategy->sess = this;
    maybe_serialize_strategies();
    return strategy;
}

Strategy::Ptr Session::add_new(const std::string& unique_id, const std::string& name, int val) {
    auto result = strategies.emplace(unique_id,
             std::make_shared<Strategy>(this, unique_id, name));
    result.first->second->set_const(val);
    maybe_serialize_strategies();
    return result.first->second;
}

Strategy::Ptr Session::add_random(const std::string& unique_id, const std::string& name) {
    auto result = strategies.emplace(unique_id,
             std::make_shared<Strategy>(this, unique_id, name));
    result.first->second->set_random();
    maybe_serialize_strategies();
    return result.first->second;
}

bool Session::remove(Strategy::Ptr strat) {
    return remove_by_id(strat->unique_id);
}

bool Session::remove_by_id(const std::string& unique_id) {
    auto it = strategies.find(unique_id);
    if (it == strategies.end()) return false;
    it->second->sess = nullptr;
    strategies.erase(it);
    maybe_serialize_strategies();
    return true;
}

void Session::clear() {
    for (auto& strat : strategies) {
        strat.second->sess = nullptr;
    }
    strategies.clear();
    maybe_serialize_strategies();
}

void Session::unlink() {
    if (!name.empty()) {
        std::string session_dir = STORAGE_ROOT + name;
        strats_path.clear();
        util::remove_dir(session_dir);
        std::cerr << "Bacon: session '" << name << "' made transient\n";
        name.clear();
    } else {
        std::cerr << "Bacon: Nothing done since that session is already transient\n";
    }
}

int Session::size() const {
    return strategies.size();
}

Strategy::Ptr Session::get(const std::string& id) const {
    auto it = strategies.find(id);
    if (it == strategies.end()) {
        throw std::out_of_range("Strategy with specified id does not exist");
    }
    return it->second;
}

bool Session::contains(const std::string& id) const {
    return strategies.count(id);
}

std::vector<Strategy::Ptr> Session::values() const {
    std::vector<Strategy::Ptr> result;
    result.reserve(strategies.size());
    for (const auto& strat_pair : strategies) {
        result.push_back(strat_pair.second);
    }
    return result;
}

std::vector<std::string> Session::keys() const {
    std::vector<std::string> result;
    result.reserve(strategies.size());
    for (const auto& strat_pair : strategies) {
        result.push_back(strat_pair.first);
    }
    return result;
}

std::vector<std::string> Session::names() const {
    std::vector<std::string> result;
    result.reserve(strategies.size());
    for (const auto& strat_pair : strategies) {
        result.push_back(strat_pair.second->name);
    }
    return result;
}

double Session::win_rate(const std::string& id0, const std::string& id1) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate(*get(id0), *get(id1));
}

double Session::win_rate0(const std::string& id0, const std::string& id1) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate_going_first(*get(id0), *get(id1));
}

double Session::win_rate1(const std::string& id0, const std::string& id1) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate_going_last(*get(id0), *get(id1));
}

Results::Ptr Session::run(int num_threads, bool quiet) {
    auto new_results = std::make_shared<Results>();

    // Create unique-id-to-index map and initialize results
    std::map<std::string, int> id_map;
    int index = 0;
    new_results->table.resize(strategies.size());
    for (auto & strat : strategies) {
        id_map[strat.second->unique_id] = index;
        new_results->strategies.push_back(std::make_shared<Strategy>());
        // Make copy but detach from session
        *new_results->strategies.back() = *strat.second;
        new_results->table[index].resize(index);
        ++index;
    }
    
    // Find any corresponding strategies from previous computation
    std::vector<int> map_to_old_strategies(strategies.size(), -1);
    if (results != nullptr) {
        for (int i = 0; i < static_cast<int>(results->strategies.size()); ++i) {
            auto& strat = results->strategies[i];
            auto strat_it = strategies.find(strat->unique_id);
            if (strat_it != strategies.end()) {
                auto& new_strat = strat_it->second;
                if (new_strat->equals(*strat)) {
                    int new_id = id_map[new_strat->unique_id];
                    map_to_old_strategies[new_id] = i;
                }
            }
        }
    }

    // Find out which matchups actually need to be recomputed
    int idx1 = 0;
    using Matchup = std::pair<int, int>;
    std::vector<Matchup> matchups;
    matchups.reserve(strategies.size());
    for (int i = 0; i < static_cast<int>(strategies.size()); ++i) {
        for (int j = 0; j < i; ++j) {
            if (~map_to_old_strategies[i] && ~map_to_old_strategies[j]) {
                new_results->table[i][j] =
                    results->get(map_to_old_strategies[i], 
                                 map_to_old_strategies[j]);
            } else {
                matchups.emplace_back(i, j );
            }
        }
    }
    if (!quiet) {
        std::cerr << "Starting, " << matchups.size() << " matches to play\n";
    }

    // Compute all matchups in parallel
    size_t matchup_index = 0;
    std::mutex mutex;
    auto worker = [&]() {
        auto core = std::unique_ptr<Core>(new Core());
        int worker_matchup_index;
        int strat0, strat1;
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (matchup_index >= matchups.size()) break;
                worker_matchup_index = matchup_index++;
                if (!quiet && matchup_index % 50 == 0) {
                    std::cerr << matchup_index << " of " <<
                        matchups.size() << " matchups played\n";
                }
            }
            strat0 = matchups[worker_matchup_index].first;
            strat1 = matchups[worker_matchup_index].second;
            new_results->table[strat0][strat1] =
                core->win_rate(
                        *new_results->strategies[strat0],
                        *new_results->strategies[strat1]);
        }
    };

    std::vector<std::thread> thread_manager;
    for (int i = 0; i < num_threads; ++i) {
        thread_manager.emplace_back(worker);
    }
    for (int i = 0; i < num_threads; ++i) {
        thread_manager[i].join();
    }

    // Output results
    results = new_results;
    results->make_rankings();
    maybe_serialize_results();
    return results;
}

std::shared_ptr<SessConfig> Session::get_config() {
    return std::make_shared<SessConfig>(*this);
}

bool Session::is_persistent() const {
    return !name.empty();
}

bool Session::load_state() {
    // Load the strategies
    std::ifstream strats_file(strats_path,
            std::ios::in | std::ios::binary);
    if (strats_file) {
        // Load the file
        uint64_t num_strats;
        util::read_bin(strats_file, num_strats);
        strategies.clear();
        while (num_strats--) {
            auto strat = std::make_shared<Strategy>(this);
            strats_file >> *strat;
            strategies[strat->unique_id] = std::move(strat);
        }
        strats_file.close();
    }

    std::ifstream results_file(results_path,
            std::ios::in | std::ios::binary);
    if (results_file) {
        uint64_t num_result_strats;
        util::read_bin(results_file, num_result_strats);
        results = std::make_shared<Results>();
        for (int i = 0; i < num_result_strats; ++i) {
            auto strat = std::make_shared<Strategy>(this);
            results_file >> *strat;
            results->strategies.push_back(std::move(strat));
        }

        results->table.resize(num_result_strats);
        for (size_t i = 0; i < results->table.size(); ++i) {
            results->table[i].resize(i);
            for (size_t j = 0; j < results->table[i].size(); ++j) {
                util::read_bin(results_file, results->table[i][j]);
            }
        }
        results->make_rankings();
        results_file.close();
    }

    std::ifstream config_file(config_path,
            std::ios::in | std::ios::binary);
    if (config_file) {
        uint64_t num_configs;
        util::read_bin(config_file, num_configs);
        while (num_configs--) {
            uint64_t key_len, value_len;
            std::string key, value;

            util::read_bin(config_file, key_len);
            key.resize(key_len);
            config_file.read(&key[0], key_len);

            util::read_bin(config_file, value_len);
            value.resize(value_len);
            config_file.read(&value[0], value_len);

            config[key] = value;
        }
        config_file.close();
    }
    return true;
}

void Session::maybe_serialize_strategies() {
    // No persistence, exit
    if (name.empty()) return;

    // Save the strategies
    std::ofstream strats_file(strats_path,
            std::ios::out | std::ios::binary);
    if (!strats_file) {
        throw new std::runtime_error("Bacon internal error: session strategies file could not be opened for writing");
    }
    util::write_bin(strats_file, static_cast<uint64_t>(strategies.size()));
    for (auto& strat_pair : strategies) {
        strats_file << *strat_pair.second;
    }
    strats_file.close();
}

void Session::maybe_serialize_results() {
    // No persistence, exit
    if (name.empty() || results == nullptr) return;

    // Save the results
    std::ofstream results_file(results_path,
            std::ios::out | std::ios::binary);
    if (!results_file) {
        throw new std::runtime_error("Bacon internal error: session results file could not be opened for writing");
    }
    
    util::write_bin(results_file,
            static_cast<uint64_t>(results->strategies.size()));
    for (auto& strategy : results->strategies) {
        results_file << *strategy;
    }

    for (size_t i = 0; i < results->table.size(); ++i) {
        for (size_t j = 0; j < results->table[i].size(); ++j) {
            util::write_bin(results_file, results->table[i][j]);
        }
    }
    results_file.close();
}

void Session::maybe_serialize_config() {
    // No persistence, exit
    if (name.empty() || config.empty()) return;

    // Save the config
    std::ofstream config_file(config_path,
            std::ios::out | std::ios::binary);
    if (!config_file) {
        throw new std::runtime_error("Bacon internal error: session config file could not be opened for writing");
    }
    util::write_bin(config_file,
            static_cast<uint64_t>(config.size()));
    for (auto& key_value : config) {
        util::write_bin(config_file,
                static_cast<uint64_t>(key_value.first.size()));
        config_file.write(key_value.first.c_str(), key_value.first.size());
        util::write_bin(config_file,
                static_cast<uint64_t>(key_value.second.size()));
        config_file.write(key_value.second.c_str(), key_value.second.size());
    }
    config_file.close();
}

// Result implementation
double Results::get(int i0, int i1) const {
    if (i0 == i1) return 0.5;
    else if (i0 < i1) return 1.0 - table[i1][i0];
    else return table[i0][i1];
}

bool Results::is_win(int i0, int i1) const {
    return get(i0, i1) > 0.5 + WIN_EPSILON;
}

std::vector<std::string> Results::keys() const {
    std::vector<std::string> list_of_keys;
    list_of_keys.reserve(strategies.size());
    for (auto & strat : strategies) {
        list_of_keys.push_back(strat->unique_id);
    }
    return list_of_keys;
}

std::vector<std::string> Results::names() const {
    std::vector<std::string> list_of_names;
    list_of_names.reserve(strategies.size());
    for (auto & strat : strategies) {
        list_of_names.push_back(strat->name);
    }
    return list_of_names;
}

std::string Results::repr() const {
    std::string output;
    output.reserve(strategies.size() * 15);
    output.append("bacon.Results(\n");
    for (const auto& strat_pair : rankings) {
        output.append(" " + strategies[strat_pair.first]->name +
                      " with " + std::to_string(strat_pair.second) +
                      " win" +
                      (strat_pair.second != 1 ? "s" : "") +
                      "\n");
    }
    output.append(")");
    return output;
}

std::string Results::str() const {
    std::string output;
    output.reserve(strategies.size() * 15);
    for (const auto& strat_pair : rankings) {
        output.append(strategies[strat_pair.first]->name +
                      " with " + std::to_string(strat_pair.second) +
                      " win" +
                      (strat_pair.second != 1 ? "s" : "") +
                      "\n");
    }
    return output;
}

void Results::make_rankings() {
    std::vector<int> wins(strategies.size(), 0);
    rankings.clear();
    for (size_t i = 0; i < strategies.size(); ++i) {
        rankings.emplace_back(i, 0);
    }
    for (size_t i = 0; i < strategies.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (table[i][j] > 0.5 + WIN_EPSILON) {
                ++rankings[i].second;
            } else if (table[i][j] < 0.5 - WIN_EPSILON) {
                ++rankings[j].second;
            }
        }
    }

    std::sort(rankings.begin(), rankings.end(), [this](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        if (a.second == b.second) {
            return strategies[a.first]->name < strategies[b.first]->name;
        }
        return a.second > b.second;
    });
}

std::string SessConfig::get(const std::string& key) const {
    auto it = sess.config.find(key);
    if (it != sess.config.end()) {
        return it->second;
    } else {
        throw std::out_of_range(std::string("Config entry '" + key + "' does not exist"));
    }
}

void SessConfig::set(const std::string& key, const std::string& value) {
    sess.config[key] = value;
    sess.maybe_serialize_config();
}

void SessConfig::remove(const std::string& key) {
    sess.config.erase(key);
    sess.maybe_serialize_config();
}

}  // namespace bacon
