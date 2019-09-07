#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>

#include "strategy.hpp"

namespace bacon {
/** Bacon contest results */
struct Results {
    typedef std::shared_ptr<Results> Ptr;

    /** Get winrate table entry */
    double get(int i0, int i1) const;

    /** Check if matchup is a win for the first strategy */
    bool is_win(int i0, int i1) const;

    /** Get a list of strategy unique ids */
    std::vector<std::string> keys() const;

    /** Get a list of strategy names */
    std::vector<std::string> names() const;

    /** Printable rankings */
    std::string repr() const;

    /** Convert to string */
    std::string str() const;

    /** Re-construct the rankings */
    void make_rankings();

    /** Stores win rates. */
    std::vector<std::vector<double> > table;

    /** Stores strategies that were in the contest. */
    std::vector<Strategy::Ptr> strategies;

    /** Store rankings */
    std::vector<std::pair<int, int> > rankings;
};

/** Config wrapper for Python use.
 *  Note: this is not actually used for config. Rather, it
 *  is a wrapper class that makes accessing settings in Python
 *  simpler. */
struct SessConfig {
    SessConfig (Session& sess) : sess(sess) {}

    /** Config accessors */
    std::string get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
    void remove(const std::string& key);

    /** Owning session */
    Session& sess;
};

/** A Bacon session */
struct Session {
    /** Create a new session.
     *  @param name if specified, creates persistent session
     *  with given name. If empty (default), creates temporary
     *  session.*/
    Session(const std::string &name);

    /** Get a list of session names */
    static std::vector<std::string> list_sessions();

    /** Add an existing strategy */
    Strategy::Ptr add(Strategy::Ptr strategy);

    /** Construct and add a strategy, setting each roll number to val */
    Strategy::Ptr add_new(const std::string& unique_id, const std::string& name = "", int val = 0);

    /** Construct and add a strategy, choosing each roll number uar */
    Strategy::Ptr add_random(const std::string& unique_id, const std::string& name = "");

    /** Remove a strategy */
    bool remove(Strategy::Ptr strategy);

    /** Remove a strategy by unique id */
    bool remove_by_id(const std::string& id);

    /** Clear strategies */
    void clear();

    /** Make the session non-persistent */
    void unlink();

    /** Returns the number of strategies */
    int size() const;

    /** Get a strategy by unique id */
    Strategy::Ptr get(const std::string& id) const;

    /** Check if contains a strategy id. */
    bool contains(const std::string& id) const;

    /** Get a list of strategies */
    std::vector<Strategy::Ptr> values() const;

    /** Get a list of strategy unique ids */
    std::vector<std::string> keys() const;

    /** Get a list of strategy names */
    std::vector<std::string> names() const;

    /** Compute win rate */
    double win_rate(const std::string& id0, const std::string& id1) const;

    /** Compute win rate with first player going first */
    double win_rate0(const std::string& id0, const std::string& id1) const;

    /** Compute win rate with second player going first */
    double win_rate1(const std::string& id0, const std::string& id1) const;

    /** Run the contest */
    Results::Ptr run(int num_threads, bool quiet = false);

    /** Get shared pointer to configuration, for Python use */
    std::shared_ptr<SessConfig> get_config();

    /** Returns true if this is a persistent session */
    bool is_persistent() const;

    /** Stores session name */
    std::string name;

    /** Contest results */
    Results::Ptr results = nullptr;

    /** Stores configurations. */
    std::map<std::string, std::string> config;

private:
    /** Stores strategies. */
    std::map<std::string, Strategy::Ptr> strategies;
    
    /** Persistence file paths */
    std::string strats_path, results_path, config_path;

    /** Deserialize the state */
    bool load_state();

    /** Serialize the strategies in the session, if persistent session */
    void maybe_serialize_strategies();

    /** Serialize the results in the session, if persistent session */
    void maybe_serialize_results();

    /** Serialize the config in the session, if persistent session */
    void maybe_serialize_config();

    friend Strategy;
    friend SessConfig; 
};
}
