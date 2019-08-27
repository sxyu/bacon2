#include "strategy.hpp"

#include <cstring>
#include <functional>
#include "util.hpp"
#include "session.hpp"

namespace bacon {

namespace {
constexpr size_t ROLLS_SIZE =
    hog::GOAL * hog::GOAL * sizeof(HogStrategy::RollType);
}


HogStrategy::HogStrategy(const HogStrategy& other) {
    (*this) = other;
}

HogStrategy::HogStrategy(Session* sess, const std::string& unique_id,
        const std::string& name)
    : sess(sess), unique_id(unique_id), name(name.empty() ? unique_id : name) { }

HogStrategy::HogStrategy(const std::string& unique_id,
        const std::string& name)
    : sess(nullptr), unique_id(unique_id), name(name.empty() ? unique_id : name) { }


HogStrategy& HogStrategy::operator=(const HogStrategy& other) {
    unique_id = other.unique_id;
    name = other.name;
    memcpy(rolls.data(), other.rolls.data(), ROLLS_SIZE);
    sess = nullptr; // must detach
    return *this;
}

int HogStrategy::num_diff(const HogStrategy& other) const {
    int ndiff = 0;
    for (int i = 0; i < hog::GOAL * hog::GOAL; ++i) {
        ndiff += (rolls[i] != other.rolls[i]);
    }
    return ndiff;
}

bool HogStrategy::equals(const HogStrategy& other) const {
    for (int i = 0; i < hog::GOAL * hog::GOAL; ++i) {
        if (rolls[i] != other.rolls[i]) return false;
    }
    return true;
}

HogStrategy::RollType HogStrategy::get(int our_score, int oppo_score) const {
    if (our_score < 0 || oppo_score < 0 || our_score >= bacon::hog::GOAL ||
            oppo_score >= bacon::hog::GOAL) {
        throw std::out_of_range("Index out of bounds");
    }
    return rolls[our_score * hog::GOAL + oppo_score];
}

void HogStrategy::set(int our_score, int oppo_score, RollType value) {
    if (our_score < 0 || oppo_score < 0 || our_score >= bacon::hog::GOAL ||
            oppo_score >= bacon::hog::GOAL) {
        throw std::out_of_range("Index out of bounds");
    }
    if(value < bacon::hog::MIN_ROLLS || value > bacon::hog::MAX_ROLLS) {
        throw std::out_of_range("Number of rolls out of bounds");
    }
    rolls[our_score * hog::GOAL + oppo_score] = value;
    if (sess) sess->maybe_serialize_strategies();
}

void HogStrategy::set_random() {
    for (int i = 0; i < hog::GOAL * hog::GOAL; ++i) {
        rolls[i] = util::randint(hog::MIN_ROLLS, hog::MAX_ROLLS);
    }
    if (sess) sess->maybe_serialize_strategies();
}

void HogStrategy::set_from_buffer(const int8_t * buf) {
    memcpy(rolls.data(), buf, ROLLS_SIZE);
    if (sess) sess->maybe_serialize_strategies();
}

void HogStrategy::set_const(RollType roll) {
    if(roll < bacon::hog::MIN_ROLLS || roll > bacon::hog::MAX_ROLLS) {
        throw std::out_of_range("Number of rolls out of bounds");
    }
    memset(rolls.data(), roll, ROLLS_SIZE);
    if (sess) sess->maybe_serialize_strategies();
}

std::ostream& operator<<(std::ostream& os, const HogStrategy& strat) {
    os.write("\n", 1);
    util::write_bin(os, static_cast<uint64_t>(strat.unique_id.size()));
    os.write(strat.unique_id.c_str(), strat.unique_id.size());
    util::write_bin(os, static_cast<uint64_t>(strat.name.size()));
    os.write(strat.name.c_str(), strat.name.size());
    os.write(reinterpret_cast<const char*>(strat.rolls.data()), ROLLS_SIZE);
    return os;
}

std::istream& operator>>(std::istream& is, HogStrategy& strat) {
    char marker;
    is.read(&marker, 1);
    if (marker != '\n') {
        throw std::runtime_error("Bacon internal error: Serialized strategies are corrupted, unable to load session. Please rebuild session.");
        return is;
    }
    uint64_t unique_id_sz;
    util::read_bin(is, unique_id_sz);
    strat.unique_id.resize(unique_id_sz);
    is.read(&strat.unique_id[0], unique_id_sz);

    uint64_t name_sz;
    util::read_bin(is, name_sz);
    strat.name.resize(name_sz);
    is.read(&strat.name[0], name_sz);

    is.read(reinterpret_cast<char *>(strat.rolls.data()), ROLLS_SIZE);
    return is;
}
}
