#pragma once
#include <random>
#include <vector>
#include <utility>
#include <iostream>

namespace bacon {
namespace util {

template<class T>
/** mersenne twister PRNG */
inline T randint_mt(T lo, T hi) {
    static std::random_device rd{};
    // some compilers (e.g. mingw g++) have bad random device implementation, must use time(NULL)
    static std::mt19937 rg(rd.entropy() > 1e-9 ? rd() : static_cast<unsigned>(time(NULL)));
    std::uniform_int_distribution<T> pick(lo, hi);
    return pick(rg);
}

template<class T>
/** xorshift-based PRNG (inclusive on both sides) */
inline T randint(T lo, T hi) {
    if (hi <= lo) return lo;
    static unsigned long x = std::random_device{}(), y = std::random_device{}(), z = std::random_device{}();
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z % (hi - lo + 1) + lo;
}

template<class T>
/** Write binary to ostream */
inline T write_bin(std::ostream& os, T val) {
    os.write(reinterpret_cast<char*>(&val), sizeof(T));
}

template<class T>
/** Read binary from istream */
inline void read_bin(std::istream& is, T& val) {
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
}

/** Trim name and add ... if over 'max_len' in length */
std::string trim_name(const std::string& name, int max_len = 40);

/** Escape a string */
std::string quote(const std::string& string);

/** Un-escape a string */
std::string unquote(const std::string& string);

/** List a directory */
std::vector<std::string> lsdir(const std::string & path);

/** Create directory */
void create_dir(const std::string& path);

/** Remove directory */
void remove_dir(const std::string& path);

}  // namespace util
}  // namespace bacon
