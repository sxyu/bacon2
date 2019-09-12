/** Python C extension bindings */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/stl_bind.h>
namespace py = pybind11;

#include <thread>
#include "session.hpp"
#include "util.hpp"

namespace {
    using bacon::Session;
    using bacon::SessConfig;
    using bacon::Results;
    using bacon::Strategy;
    using bacon::util::trim_name;
}

// C++ module definition
PYBIND11_MAKE_OPAQUE(std::vector<double>);

PYBIND11_MODULE(_bacon, m) {
    py::bind_vector<std::vector<double>>(m, "PartialRow");
    py::class_<Strategy, Strategy::Ptr>(m, "Strategy")
        .def(py::init<const std::string &, const std::string &>(),
                "Construct new strategy with id, name",
                py::arg("id"), py::arg("name") = "")
        .def("set_random", &Strategy::set_random, "Set roll numbers uniformly at random") 
        .def("set_const", &Strategy::set_const, "Set roll numbers to a constant") 
        .def("set_optimal", &Strategy::set_optimal, "Set to optimal strategy (only optimal if time trot/feral hogs disabled)") 
        .def("clone", [](Strategy& strat) {
                    Strategy::Ptr new_strat(new Strategy);
                    *new_strat = strat;
                    return new_strat;
                }, "Clone the strategy") 
        .def("train", &Strategy::train, "Begin hill climbing against an opponent for given number of turns (opponent can be self)") 
        .def("win_rate", &Strategy::win_rate, "Compute win rate against opponent") 
        .def("win_rate0", &Strategy::win_rate0, "Compute win rate against opponent, going first") 
        .def("win_rate1", &Strategy::win_rate1, "Compute win rate against opponent, going second") 
        .def("num_diff", &Strategy::num_diff, "Find number of differences to another strategy")
        .def("equals", &Strategy::equals, "Checks if exactly equal to another strategy")
        .def("array", [](Strategy& strat) {
            return py::array_t<Strategy::RollType>(
                py::buffer_info(
                    strat.rolls.data(),
                    sizeof(Strategy::DATA_SIZE), //itemsize
                    py::format_descriptor<Strategy::RollType>::format(),
                    2, // ndim
                    std::vector<size_t> { Strategy::DATA_SIZE, Strategy::DATA_SIZE }, // shape
                    std::vector<size_t> { Strategy::DATA_SIZE  * sizeof(Strategy::RollType), sizeof(Strategy::RollType)} // strides
                    )
                );
        }, "Get a Numpy array of roll numbers")
        .def("set_array", [](Strategy& strat, py::array_t<Strategy::RollType, py::array::c_style | py::array::forcecast>& arr) {
            strat.set_from_buffer(arr.data());
        }, "Set from a Numpy array of roll numbers")
        .def("draw", &Strategy::draw, "Draw the strategy diagram, just as in the original Bacon")
        .def_readwrite("name", &Strategy::name, "The strategy name (readonly)")
        .def_readonly("id", &Strategy::unique_id, "The strategy unique id (readonly)")
        .def("__getitem__", [](Strategy& strat, py::tuple& tup) {
                if(tup.size() != 2)
                     throw std::invalid_argument("2 arguments: (our score, oppo score) expected");
                return strat.get(tup[0].cast<int>(), tup[1].cast<int>());
            }, "Slice operator")
        .def("__setitem__", [](Strategy& strat, py::tuple& tup, int value) {
                if(tup.size() != 2) throw std::invalid_argument("2 arguments: (our score, oppo score) expected");
                int r = tup[0].cast<int>(), c = tup[1].cast<int>();
                strat.set(r, c, static_cast<Strategy::RollType>(value));
            }, "Slice operator")
        .def("__repr__", [](Strategy& strat){
                return "bacon.Strategy('" + strat.unique_id + "', name = '" + trim_name(strat.name, 40) + "')";
            })
    ;
    
    py::class_<SessConfig, std::shared_ptr<SessConfig> >(m, "SessionConfig")
        .def("__getitem__", &SessConfig::get, "Get operator, throws IndexError if not present")
        .def("__setitem__", &SessConfig::set, "Set operator")
        .def("remove", &SessConfig::remove, "Remove config entry by name, does nothing if not present")
        .def("__contains__", [](SessConfig& config, const std::string& name) {
            return config.sess.config.count(name) == 1;
        }, "In operator")
        .def("__len__", [](SessConfig& config) {
            return config.sess.config.size();
        }, "Number of config options set")
        .def("__repr__", [](SessConfig& config) {
                    std::string result = "bacon.SessionConfig {";
                    for (auto& key_value: config.sess.config) {
                        result.append(key_value.first);
                        result.append(": ");
                        result.append(trim_name(key_value.second, 40));
                        result.append(", ");
                    }
                    if (!config.sess.config.empty()) {
                        result.pop_back();
                        result.pop_back();
                    }
                    result.push_back('}');
                    return result;
                }, "Show string representation")
    ;

    py::class_<Results, Results::Ptr>(m, "Results")
        .def("is_win", &Results::is_win, "Checks if a matchup is a win")
        .def("ids", &Results::keys, "Get list of strategy unique ids (inefficient)")
        .def("names", &Results::names, "Get list of strategy names (inefficient)")
        .def("__getitem__", [](Results& results, size_t index) {
                if (index < 0 || index >= results.table.size()) {
                     throw std::out_of_range("Index out of bounds");
                }
                return results.table[index];
            }, "Slice operator, get triangular vector of win rates for a strategy index")
        .def("__getitem__", [](Results& results, py::tuple& tup) {
                if(tup.size() != 2) {
                     throw std::invalid_argument("2 arguments expected");
                 }
                return results.get(tup[0].cast<int>(), tup[1].cast<int>());
            }, "Slice operator, gets win rate between strategies")
        .def("array", [](Results& results) {
            py::array_t<double> arr({results.strategies.size(), results.strategies.size()});
            for (size_t r = 0 ; r < results.strategies.size(); ++r) {
                for (size_t c = 0 ; c < results.strategies.size(); ++c) {
                    arr.mutable_at(r, c) = results.get(r, c);
                }
            }
            return arr;
         }, "Get a Numpy array of win rates")
        .def("__repr__", &Results::repr)
        .def("__str__", &Results::str)
        .def_readonly("rankings", &Results::rankings, "Get contest rankings")
        .def_readonly("strategies", &Results::strategies, "Get list of strategies")
    ;

    py::class_<Session>(m, "Session")
        .def(py::init<const std::string &>(), "Constructor",
                py::arg("name") = "")
        .def("add", &Session::add, "Add a strategy to this session. If the strategy is already assigned to a session, this will error.")
        .def("add", &Session::add_new, "Construct a strategy with the given name and add it to the session. All roll numbers are 0 by default; specify argument rolls=x to set to x instead.", py::arg("unique_id"), py::arg("name") = "", py::arg("rolls") = 0)
        .def("add_random", &Session::add_random, "Construct a strategy with the given name and add it to the session. All rolls are chosen uniformly at random.", py::arg("unique_id"), py::arg("name") = "")
        .def("remove", &Session::remove, "Remove a strategy from the session.")
        .def("remove", &Session::remove_by_id, "Remove a strategy from the session.")
        .def("clear", &Session::clear, "Clear all strategies.")
        .def("unlink", &Session::unlink, "Unlink the session from persistent storage, making it transient. It will be destropyed once you exit python.")
        .def("win_rate", &Session::win_rate, "Compute win rate of a strategy against another")
        .def("win_rate0", &Session::win_rate0, "Compute win rate with the first strategy always going first")
        .def("win_rate1", &Session::win_rate1, "Compute win rate with the second strategy always going last")
        .def("run", &Session::run, "Run the contest with the given number of threads. A bacon.Results object is returned.",
                py::arg("num_threads") = std::thread::hardware_concurrency(),
                py::arg("quiet") = false)
        .def("is_persistent", &Session::is_persistent, "Checks whether this is a persistent (named) session")
        .def("has_results", [](Session& sess){return sess.results != nullptr;}, "Checks whether the session has results")
        .def("config", [](Session& sess){return sess.get_config();}, "Get the config map for the session")
        .def("results", [](Session& sess){
            if (sess.results == nullptr) {
                throw std::runtime_error("Results are not available for this session, please run the contest with run() first");
            }
            return sess.results;
        }, "Get the session's results")
        .def_readonly("name", &Session::name, "The session name (readonly)")
        .def("ids", &Session::keys, "Get list of strategy unique ids (inefficient)")
        .def("names", &Session::names, "Get list of strategy namess (inefficient)")
        .def("strategies", &Session::values, "Get list of strategies (inefficient)")
        .def("__len__", &Session::size, "Get number of strategies")
        .def("__getitem__", &Session::get, "Get a strategy by id.")
        .def("__contains__", &Session::contains, "Check if contains a strategy id.")
        .def("__repr__", [](Session& sess){
                if (sess.name.empty()) {
                    return "bacon.Session(transient session with " + std::to_string(sess.size()) + " strategies)";
                } else {
                    return "bacon.Session(persistent session '" + trim_name(sess.name, 40) + "' with " + std::to_string(sess.size()) + " strategies)";
                }
            })
    ;
    m.def("sessions", &Session::list_sessions, "Get a list of all persistent sessions");
    auto config_m =  m.def_submodule("config");
    config_m.attr("DICE_SIDES") = bacon::hog::DICE_SIDES;
    config_m.attr("GOAL") = bacon::hog::GOAL;
    config_m.attr("MOD_TROT") = bacon::hog::MOD_TROT;
    config_m.attr("MAX_ROLLS") = bacon::hog::MAX_ROLLS;
    config_m.attr("MIN_ROLLS") = bacon::hog::MIN_ROLLS;
    config_m.attr("FERAL_HOGS_ABSDIFF") = bacon::hog::FERAL_HOGS_ABSDIFF;
    config_m.attr("ENABLE_TIME_TROT") = bacon::hog::ENABLE_TIME_TROT;
    config_m.attr("ENABLE_SWINE_SWAP") = bacon::hog::ENABLE_SWINE_SWAP;
    config_m.attr("ENABLE_FERAL_HOGS") = bacon::hog::ENABLE_FERAL_HOGS;
    config_m.attr("WIN_EPSILON") = bacon::WIN_EPSILON;
    config_m.def("swine_swap", bacon::hog::is_swap);
    config_m.def("free_bacon", bacon::hog::free_bacon);
    m.doc() = "Bacon 2: Hog Contest Engine C++ extension";

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
