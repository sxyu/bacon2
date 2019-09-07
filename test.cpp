#include <chrono>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include "config.hpp"

// Poor man's test framework
#define BEGIN_TEST bool __passing = true
#define END_TEST(name) do{if (__passing) printf("TEST " #name " PASSED\n"); else printf("TEST " #name " FAILED\n"); return __passing;}while(false)
#define EXPECT_FALSE(x) do{if(x) { printf("FAILED: expected " #x " to be false but it is true\n"); __passing = false; } } while(false)
#define EXPECT_TRUE(x) do{if(!(x)) { printf("FAILED: expected " #x " to be true but it is false\n"); __passing = false; } } while(false)
#define EXPECT_EQ(x, y) do{if((x)!=(y)) { printf("FAILED: expected " #x " == " #y " but %s != %s\n", std::to_string(x).c_str(), std::to_string(y).c_str()); __passing = false; } } while(false)

// Profiling utils
#define BEGIN_PROFILE auto start = std::chrono::high_resolution_clock::now()
#define PROFILE(x) do{printf("%s: %f ms\n", #x, std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count()); start = std::chrono::high_resolution_clock::now(); }while(false)

namespace {
bool test_free_bacon() {
    BEGIN_TEST;
    EXPECT_EQ(bacon::hog::free_bacon(35), 7);
    EXPECT_EQ(bacon::hog::free_bacon(71), 9);
    EXPECT_EQ(bacon::hog::free_bacon(9), 10);
    EXPECT_EQ(bacon::hog::free_bacon(37), 7);
    EXPECT_EQ(bacon::hog::free_bacon(0), 10);
    EXPECT_EQ(bacon::hog::free_bacon(75), 5);
    EXPECT_EQ(bacon::hog::free_bacon(99), 1);
    EXPECT_EQ(bacon::hog::free_bacon(87), 3);
    EXPECT_EQ(bacon::hog::free_bacon(13), 9);
    EXPECT_EQ(bacon::hog::free_bacon(85), 5);
    EXPECT_EQ(bacon::hog::free_bacon(7), 10);
    EXPECT_EQ(bacon::hog::free_bacon(8), 10);
    EXPECT_EQ(bacon::hog::free_bacon(80), 10);
    EXPECT_EQ(bacon::hog::free_bacon(55), 5);
    END_TEST(FreeBaconTest);
}

bool test_is_swap() {
    BEGIN_TEST;
    EXPECT_FALSE(bacon::hog::is_swap(2, 4));
    EXPECT_FALSE(bacon::hog::is_swap(22, 4));
    EXPECT_TRUE(bacon::hog::is_swap(28, 4));
    EXPECT_TRUE(bacon::hog::is_swap(124, 2));
    EXPECT_TRUE(bacon::hog::is_swap(44, 28));
    EXPECT_FALSE(bacon::hog::is_swap(2, 0));
    EXPECT_TRUE(bacon::hog::is_swap(10, 0));
    EXPECT_TRUE(bacon::hog::is_swap(100, 10));
    EXPECT_TRUE(bacon::hog::is_swap(14, 2));
    EXPECT_TRUE(bacon::hog::is_swap(27, 72));
    EXPECT_TRUE(bacon::hog::is_swap(104, 2));
    EXPECT_TRUE(bacon::hog::is_swap(66, 6));
    EXPECT_TRUE(bacon::hog::is_swap(11, 1));
    EXPECT_TRUE(bacon::hog::is_swap(13, 301));
    END_TEST(IsSwapTest);
}
}  // namespace

int main(void) {
    bool all_pass = false;
    all_pass |= test_free_bacon();
    all_pass |= test_is_swap();
    if (all_pass) {
        printf("All tests passed :)\n");
    } else {
        printf("Heads-up: some tests failed!\n");
    }
}



