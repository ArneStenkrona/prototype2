#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/vector.h"

#include <iostream>

TEST_CASE( "Test vector", "[Vector]") {
    prt::vector<uint32_t> vec1;
    for (size_t i = 0; i < 100; i++) {
        vec1.push_back(i*i + i);
    }
    for (size_t i = 0; i < 100; i++) {
        REQUIRE(vec1[i] == i*i + i);
    }

    prt::vector<uint32_t> vec2;
    vec2.resize(10000);
    for (size_t i = 0; i < 10000; i++) {
        std::cout << i << std::endl;
        vec2[i] = i*i + i;
    }
    for (size_t i = 0; i < 10000; i++) {
        REQUIRE(vec2[i] == i*i + i);
    }
}