#pragma once
#include "config.hpp"

namespace bacon {
class HogStrategy;
struct HogCore {
    HogCore();

    /** Compute exact average win rate between two strategies */
    double win_rate(const HogStrategy& strat, const HogStrategy& oppo_strat);
    /** Compute exact average win rate between two strategies, with first strategy always playing first */
    double win_rate_going_first(const HogStrategy& strat, const HogStrategy& oppo_strat);

    /** Compute exact average win rate between two strategies, with second strategy always playing first */
    double win_rate_going_last(const HogStrategy& strat, const HogStrategy& oppo_strat);

private:
    /** Recursive helper for computing win rate */
    double compute_win_rate_recursive(const HogStrategy& strat, const HogStrategy & oppo_strat, int score, int oppo_score, int who, int turn, int trot);

    /** Helper for clearing win rates */
    void clear_win_rates();

    /** DP storage */
    double win_rates[hog::GOAL][hog::GOAL][2][hog::MOD_TROT][2];
};

// Note: following line is defined so that in the future
// we may swap out HogCore for something else with the
// same interface
using Core = HogCore;

}
