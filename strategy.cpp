#include "strategy.hpp"

#include <cstring>
#include <functional>
#include <memory>
#include "util.hpp"
#include "session.hpp"
#include "core.hpp"

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
    : sess(nullptr), unique_id(unique_id), name(name.empty() ? unique_id : name) {
        if (unique_id.empty()) {
            throw new std::invalid_argument("Strategy name cannot be empty");
        }
        // Since this function is exposed to Python, we can't keep uninitialized memory
        // (else would seg fault when used)
        set_const(0);
    }


HogStrategy& HogStrategy::operator=(const HogStrategy& other) {
    unique_id = other.unique_id;
    name = other.name;
    memcpy(rolls.data(), other.rolls.data(), ROLLS_SIZE);
    sess = nullptr; // must detach
    return *this;
}

HogStrategy::Ptr HogStrategy::clone(const std::string& id, const std::string& name) {
    HogStrategy::Ptr cloned(new HogStrategy(id, name));
    memcpy(cloned->rolls.data(), rolls.data(), ROLLS_SIZE);
    return cloned;
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

void HogStrategy::set_optimal() {
    Core::make_optimal_strategy(*this);
}

void HogStrategy::train(HogStrategy::Ptr opponent, int num_steps) {
    std::unique_ptr<Core> core (new Core);
    core->train_strategy(*this, *opponent, num_steps);
}

void HogStrategy::train_greedy(HogStrategy::Ptr opponent, int num_steps) {
    std::unique_ptr<Core> core (new Core);
    core->train_strategy_greedy(*this, *opponent, num_steps);
}

double HogStrategy::win_rate(HogStrategy::Ptr opponent) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate(*this, *opponent);
}

double HogStrategy::win_rate0(HogStrategy::Ptr opponent) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate_going_first(*this, *opponent);
}

double HogStrategy::win_rate1(HogStrategy::Ptr opponent) const {
    auto core = std::unique_ptr<Core>(new Core());
    return core->win_rate_going_last(*this, *opponent);
}

void HogStrategy::draw() {
    std::cout << "Y-axis is player score, X-axis is opponent score. Bottom left is 0, 0.\n" << std::endl;
    for (int i = hog::GOAL - 2; i >= 0; i -= 2) {
        std::cout << "[";
        for (int j = 0; j < hog::GOAL; ++j) {
            int result = (get(i, j) + get(i + 1, j)) / 2;
            switch (result) {
            case 0:
                std::cout << " ";
                break;
            case 1:
            case 2:
                std::cout << ":";
                break;
            case 3:
            case 4:
                std::cout << "|";
                break;
            case 5:
            case 6:
                std::cout << "%";
                break;
            case 7:
            case 8:
                std::cout << "\u2593";
                break;
            case 9:
            case 10:
                std::cout << "\u2588";
                break;
            default:
                std::cout << "??";
            }
        }
        std::cout << "]" << std::endl;
    }

    std::cout << "\nLEGEND:  [  ] = 0  [::] = 1,2  [||] = 3,4  [%%] = 5,6  [\u2593\u2593] = 7,8  [\u2588\u2588] = 9,10." << std::endl;
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
