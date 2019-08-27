#include "config.hpp"

#include <algorithm>
namespace bacon {
namespace hog {

// Free bacon rule definition
int free_bacon(int score) {
    // SP19 rules
    //return std::min((score / 10 % 10), score % 10) + 1;

    // FA18 rules
    return std::max(2 * (score / 10 % 10) - score % 10, 1);

    /*
    // SP18 rules
    int max_digit = 0;

    while (score > 0) {
    max_digit = std::max(score % 10, max_digit);
    score /= 10;
    }
    return max_digit + 1;
    */
}

// swine swap rule definition
bool is_swap(int score0, int score1) {
    // SP19 rules
    /*
    for (; score0 && score1; score0 /= 10, score1 /= 10) {
        if (score0 % 10 == score1 % 10) {
            return true;
        }
    }
    return false;
    */

    // FA18 rules
    return abs(score0 / 10 % 10 - score0 % 10) == abs(score1 / 10 % 10 - score1 % 10);
    /*
    // SP18 rules
    if (score0 <= 1 || score1 <= 1) return false;
    return score0 % score1 == 0 || score1 % score0 == 0;
    */
}

}
}
