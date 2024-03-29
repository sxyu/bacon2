#include "core.hpp"

#include <cstring>
#include <limits>
#include <iostream>
#include <memory>
#include "strategy.hpp"
#include "util.hpp"

namespace bacon {
namespace {
// wtsfr[i][j]: # ways to get sum j by rolling i dice
// A bit ugly but more convenience to use
int ways_to_sum_for_rolls[hog::MAX_ROLLS + 1][hog::DICE_SIDES * hog::MAX_ROLLS + 1];

void precompute() {
    if (!ways_to_sum_for_rolls[0][0]) {
        // Precompute once, copied from Bacon v1
        ways_to_sum_for_rolls[0][0] = 1; // base case
        for (int i = 1; i <= hog::MAX_ROLLS; ++i) {
            // the lowest possible sum for the previous number of rolls
            int prev_low = 2 * (i - 1);
            // add # ways of getting a score of one due to Pig Out
            int num_ones = 0, last_pow = 1, last_choose = 1;

            for (int j = 1; j <= i; ++j) {
                num_ones += last_choose * last_pow;
                last_pow *= 5;
                last_choose = last_choose * (i - j + 1) / j;
            }
            ways_to_sum_for_rolls[i][1] += num_ones;

            // sum up permutations for getting all other scores
            // rolling sum to reduce complexity by m (DICE_SIDES)
            int rolling = 0;
            for (int j = hog::DICE_SIDES * i - hog::DICE_SIDES + 1; j < hog::DICE_SIDES * i; ++j) {
                rolling += ways_to_sum_for_rolls[i - 1][j];
            }
            for (int j = hog::DICE_SIDES * i; j >= 2 * i; --j) {
                // update rolling sum
                if (j - 1 >= prev_low) rolling -= ways_to_sum_for_rolls[i - 1][j - 1];
                if (j - hog::DICE_SIDES >= prev_low) rolling += ways_to_sum_for_rolls[i - 1][j - hog::DICE_SIDES];
                ways_to_sum_for_rolls[i][j] = rolling;
            }
        }
    }
}
}  // namespace

HogCore::HogCore(bool enable_time_trot, bool enable_feral_hogs, bool enable_swine_swap) :
                enable_time_trot(enable_time_trot),
                enable_feral_hogs(enable_feral_hogs),
                enable_swine_swap(enable_swine_swap) {
    precompute();
}

double HogCore::win_rate(const HogStrategy& strat, const HogStrategy& oppo_strat) {
    clear_win_rates();
    double wr0 = compute_win_rate_recursive(strat, oppo_strat, 0, 0, 0, 0, 0, 0, enable_time_trot);
    double wr1 = 1.0 - compute_win_rate_recursive(oppo_strat, strat, 0, 0, 1, 0, 0, 0, enable_time_trot);
    return (wr0 + wr1) * 0.5;
}

double HogCore::win_rate_going_first(const HogStrategy& strat, const HogStrategy& oppo_strat) {
    clear_win_rates();
    return compute_win_rate_recursive(strat, oppo_strat, 0, 0, 0, 0, 0, 0, enable_time_trot);
}

double HogCore::win_rate_going_last(const HogStrategy& strat, const HogStrategy& oppo_strat) {
    clear_win_rates();
    return 1.0 - compute_win_rate_recursive(oppo_strat, strat, 0, 0, 0, 0, 0, 0, enable_time_trot);
}

void HogCore::train_strategy(HogStrategy& strat, const HogStrategy& opponent, int num_steps) {
    // This is probably messed up, I haven't really used it, so beware
    int steps = 0;
    // We use a clone because maybe strat == opponent
    HogStrategy clone = strat;
    while (steps < num_steps) {
        for (int i = 0; i < hog::GOAL; ++i) {
            for (int j = 0; j < hog::GOAL; ++j) {
                if (steps % 500 == 499) std::cerr << "bacon.hog_core.train_strategy: " << steps + 1 << " steps completed\n";
                double best_wr = std::numeric_limits<double>::min();
                int best_roll = 0;
                for (int rolls = hog::MIN_ROLLS; rolls <= hog::MAX_ROLLS; ++rolls) {
                    clone.set(i, j, rolls);
                    double new_wr = win_rate(clone, opponent);
                    if (new_wr > best_wr) {
                        best_wr = new_wr;
                        best_roll = rolls;
                    }
                }
                strat.set(i, j, best_roll);
                clone.set(i, j, best_roll);
                if (++steps >= num_steps) break;
            }
            if (steps >= num_steps) break;
        }
    }
}

void HogCore::train_strategy_greedy(HogStrategy& strat, const HogStrategy& opponent, int num_steps) {
    // This is probably messed up, I haven't really used it, so beware
    int steps = 0;
    clear_win_rates();
    compute_win_rate_recursive(strat, opponent, 0, 0, 0, 0, 0, 0, enable_time_trot);
    while (steps < num_steps) {
        for (int i = 0; i < hog::GOAL; ++i) {
            for (int j = 0; j < hog::GOAL; ++j) {
                if (steps % 500 == 499) std::cerr << "bacon.hog_core.train_strategy: " << steps + 1 << " steps completed\n";
                double best_wr = std::numeric_limits<double>::min();
                int best_roll = 0;
                for (int rolls = hog::MIN_ROLLS; rolls <= hog::MAX_ROLLS; ++rolls) {
                    strat.set(i, j, rolls);
                    win_rates[i][j][0][0][0][0][enable_time_trot] = 0.;
                    double new_wr = compute_win_rate_recursive(strat, opponent, i, j, 0, 0, 0, 0, enable_time_trot);
                    if (new_wr > best_wr) {
                        best_wr = new_wr;
                        best_roll = rolls;
                    }
                }
                strat.set(i, j, best_roll);
                if (++steps >= num_steps) break;
            }
            if (steps >= num_steps) break;
        }
    }
}

void HogCore::make_optimal_strategy(HogStrategy& strat) {
    std::unique_ptr<HogCore> core(new HogCore(false, false, true));
    core->clear_win_rates();
    for (int t = hog::GOAL + hog::GOAL - 2; t >= 0; --t) {
        for (int j = std::max(t - hog::GOAL + 1, 0); j <= std::min(t, hog::GOAL - 1); ++j) {
            int i = t - j;
            double best_wr = std::numeric_limits<double>::min();
            int best_roll = 0;
            for (int rolls = hog::MIN_ROLLS; rolls <= hog::MAX_ROLLS; ++rolls) {
                strat.set(i, j, rolls);
                core->win_rates[i][j][0][0][0][0][0] = 0.;
                double new_wr = core->compute_win_rate_recursive(strat, strat, i, j, 0, 0, 0, 0, 0);
                if (new_wr > best_wr) {
                    best_wr = new_wr;
                    best_roll = rolls;
                }
            }
            strat.set(i, j, best_roll);
            core->win_rates[i][j][0][0][0][0][0] = best_wr + 1.0;
        }
    }
}

bool HogCore::play_one_game(const HogStrategy& strategy0, const HogStrategy & strategy1) {
    auto take_turn = [](int num_rolls, int opponent_score) {
        if (num_rolls == 0)
            return hog::free_bacon(opponent_score);

        int total = 0;
        while (num_rolls--) {
            int outcome = util::randint(1, hog::DICE_SIDES);
            if (outcome == 1) return 1;
            total += outcome;
        }
        return total;
    };

    int player = 0;
    bool last_trot = false;
    int round_number_mod = 0;
    int score0 = 0, score1 = 0;
    int previous_num_rolls_0 = 0, previous_num_rolls_1 = 0;
    while (std::max(score0, score1) < hog::GOAL) {
        int num_rolls, outcome;
        if (player == 0) {
            num_rolls = strategy0.get(score0, score1);
            outcome = take_turn(num_rolls, score1);
            score0 += outcome;
            if (enable_feral_hogs && std::abs(previous_num_rolls_0 - num_rolls) == 2) {
                score0 += 3;
            }
            previous_num_rolls_0 = num_rolls;
        } else {
            num_rolls = strategy1.get(score1, score0);
            outcome = take_turn(num_rolls, score0);
            score1 += outcome;
            if (enable_feral_hogs && std::abs(previous_num_rolls_1 - num_rolls) == 2) {
                score1 += 3;
            }
            previous_num_rolls_1 = num_rolls;
        }

        if (enable_swine_swap && hog::is_swap(score1, score0)) {
            std::swap(score0, score1);
        }
        if (!enable_time_trot || round_number_mod != num_rolls || last_trot) {
            player ^= 1;
            last_trot = false;
        } else {
            last_trot = true;
        }
        (round_number_mod += 1) %= hog::MOD_TROT;
    }
    return score0 > score1;
}

double HogCore::win_rate_by_sampling(const HogStrategy& strat, const HogStrategy& oppo_strat, int half_num_samples) {
    int wins = 0;
    for (int i = 0; i < half_num_samples; ++i) {
        wins += play_one_game(strat, oppo_strat);
    }
    for (int i = 0; i < half_num_samples; ++i) {
        wins += !play_one_game(oppo_strat, strat);
    }
    return static_cast<double>(wins) / (2 * half_num_samples);
}

double HogCore::compute_win_rate_recursive(const HogStrategy& strat, const HogStrategy & oppo_strat, int score, int oppo_score, int who, int last_rolls, int oppo_last_rolls, int turn, int trot) {
    // If some rules are disabled then we can collapse some dimensions in the state
    if (!enable_feral_hogs) {
        last_rolls = oppo_last_rolls = 0;
    }
    if (!enable_time_trot) {
        turn = trot = 0;
    }
    double& win_rate = win_rates[score][oppo_score][who][last_rolls][oppo_last_rolls][turn][trot];
    if (win_rate == 0.0) {
        int rolls = strat.get(score, oppo_score);
        // Take turn with score increase of k
        auto take_turn = [&](int k) {
            int new_score = score + k, new_oppo_score = oppo_score;
            if (enable_feral_hogs) {
                if (std::abs(rolls - last_rolls) == hog::FERAL_HOGS_ABSDIFF)
                    new_score += 3;
            }
            if (enable_swine_swap && hog::is_swap(new_score, new_oppo_score)) {
                std::swap(new_score, new_oppo_score);
            }
            double delta;
            if (new_score >= hog::GOAL) {
                delta = 1.0; // immediate win, yay
            } else if (new_oppo_score >= hog::GOAL) {
                delta = 0.0; // immediate loss due to swapping!
            } else {
                // no one wins, add win rate at next round
                if (trot && turn == rolls) {
                    // apply Time Trot
                    delta =
                        compute_win_rate_recursive(strat, oppo_strat, 
                                new_score, new_oppo_score, who, rolls, oppo_last_rolls, (turn + 1) % hog::MOD_TROT, 0);
                } else {
                    // no Time Trot, go to opponent's round
                    delta = 1.0 - compute_win_rate_recursive(oppo_strat, strat,
                            new_oppo_score, new_score, who ^ 1, oppo_last_rolls, rolls, (turn + 1) % hog::MOD_TROT, 1); 
                }
            }
            return delta;
        };

        if (rolls == 0) {
            win_rate = take_turn(hog::free_bacon(oppo_score));
        } else {
            win_rate += take_turn(1) * ways_to_sum_for_rolls[rolls][1];
            int total_times_score_counted = ways_to_sum_for_rolls[rolls][1];
            for (int k = 2*rolls; k <= hog::DICE_SIDES * rolls; ++k) {
                win_rate += take_turn(k) * ways_to_sum_for_rolls[rolls][k];
                // add to total so we can divide by this later.
                total_times_score_counted += ways_to_sum_for_rolls[rolls][k];
            }
            win_rate /= total_times_score_counted;
        }
        win_rate += 1.0;
    }
    return win_rate - 1.0;
}

void HogCore::clear_win_rates() {
    memset(win_rates, 0, sizeof win_rates);
}
}
