#pragma once
#include <ios>
#include <memory>
#include <array>
#include "config.hpp"

namespace bacon {
struct Session;

/** A hog strategy */
struct HogStrategy {
    typedef int8_t RollType;
    typedef std::shared_ptr<HogStrategy> Ptr;

    static const int DATA_SIZE = hog::GOAL;

    /** The primary constructor: create strategy with unique ID + name.
     *  If name is empty then unique id is used as name.
     *  Note that unique id should be normal ascii characters
     *  with no spaces, whereas name can have anything */
    explicit HogStrategy(Session* sess, const std::string& unique_id = "", const std::string& name = "");

    /** Constructor for creating detached strategy (not assigned to session) */
    explicit HogStrategy(const std::string& unique_id = "", const std::string& name = "");

    // Standard constructors
    HogStrategy(const HogStrategy& other);

    // Standard assignment operators
    HogStrategy& operator=(const HogStrategy& other);

    /** Set all rolls to constant */
    void set_const(RollType roll);

    /** Set each roll to a random value */
    void set_random();

    /** Set from buffer */
    void set_from_buffer(const int8_t * buf);

    /** Get number of differences to another strategy */
    int num_diff(const HogStrategy& other) const;

    /** Checks if exactly equal to other strategy */
    bool equals(const HogStrategy& other) const;

    /** Get number of rolls (const version) */
    RollType get(int our_score, int oppo_score) const;

    /** Set number of rolls safely */
    void set(int our_score, int oppo_score, RollType value);

    /** Draw the strategy diagram as in Bacon 1 */
    void draw();

    /** Pointer to session */
    Session* sess;

    /** The strategy's unique id */
    std::string unique_id;

    /** Strategy name, do not modify except for internal reasons */
    std::string name;

    /** Data array */
    std::array<RollType, hog::GOAL * hog::GOAL> rolls;
};

/** IO */
std::ostream& operator<<(std::ostream& os, const bacon::HogStrategy& strat);
std::istream& operator>>(std::istream& os, bacon::HogStrategy& strat);

// Note: following line is defined so that in the future
// we may swap out HogStrategy for something else with the
// same interface
using Strategy = HogStrategy;
}
