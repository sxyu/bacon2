#pragma once
#include "config.hpp"

namespace bacon {
class HogStrategy;
struct HogCore {
    /** Constructor. You may specify if to enable each special rule.
     *  By default uses config.hpp values */
    explicit HogCore(bool enable_time_trot = hog::ENABLE_TIME_TROT,
                     bool enable_feral_hogs = hog::ENABLE_FERAL_HOGS,
                     bool enable_swine_swap = hog::ENABLE_SWINE_SWAP);

    /** Compute exact average win rate between two strategies */
    double win_rate(const HogStrategy& strat, const HogStrategy& oppo_strat);

    /** Compute exact average win rate between two strategies, with first strategy always playing first */
    double win_rate_going_first(const HogStrategy& strat, const HogStrategy& oppo_strat);

    /** Compute exact average win rate between two strategies, with second strategy always playing first */
    double win_rate_going_last(const HogStrategy& strat, const HogStrategy& oppo_strat);

    /** Train a strategy using hill climbing */
    void train_strategy(HogStrategy& strat, const HogStrategy& opponent, int num_steps = 1000);

    /** Train a strategy using local hill climbing (very fast but can make strategy worse) */
    void train_strategy_greedy(HogStrategy& strat, const HogStrategy& opponent, int num_steps = 1000);

    /** Build the optimal strategy (only optimal without time trot/feral hogs */
    static void make_optimal_strategy(HogStrategy& strat);

private:
    /** Recursive helper for computing win rate */
    double compute_win_rate_recursive(const HogStrategy& strat, const HogStrategy & oppo_strat, int score, int oppo_score, int who, int last_rolls, int oppo_last_rolls, int turn, int trot);

    /** Helper for clearing win rates */
    void clear_win_rates();

    /** DP storage */
    double win_rates[hog::GOAL][hog::GOAL][2]
                    [hog::MAX_ROLLS - hog::MIN_ROLLS + 1]
                    [hog::MAX_ROLLS - hog::MIN_ROLLS + 1][hog::MOD_TROT][2];

    bool enable_time_trot, enable_feral_hogs, enable_swine_swap;
};

// Note: following line is defined so that in the future
// we may swap out HogCore for something else with the
// same interface
using Core = HogCore;

}
