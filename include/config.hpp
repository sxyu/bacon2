#pragma once

/** Game configuration */
namespace bacon {
namespace hog {
// Number of sides on a default die
const int DICE_SIDES = 6;

// Min valid number of times to roll
const int MIN_ROLLS = 0;

// Max valid number of times to roll
const int MAX_ROLLS = 10;

// The goal score
const int GOAL = 100;

// Time trot modulus 
const int MOD_TROT = 5;

// Feral hogs required absolute roll difference
const int FERAL_HOGS_ABSDIFF = 2;

// Whether time trot is enabled
const bool ENABLE_TIME_TROT = false;

// Whether swine swap is enabled
const bool ENABLE_SWINE_SWAP = true;

// Whether feral hogs is enabled
const bool ENABLE_FERAL_HOGS = true;


// Free bacon rule definition (edit in config.cpp)
int free_bacon(int score);

// Swine swap rule definition (edit in config.cpp)
bool is_swap(int score0, int score1);
}

/** Minimum win rate difference (0.5+-) to consider a match a win */
const double WIN_EPSILON = 0.000001;
}
