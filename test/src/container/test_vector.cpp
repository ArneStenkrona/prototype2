#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/vector.h"

#include <string>

TEST_CASE( "vector: Test vector", "[vector]") {
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
        vec2[i] = i*i + i;
    }
    for (size_t i = 0; i < 10000; i++) {
        REQUIRE(vec2[i] == i*i + i);
    }
}

TEST_CASE( "vector: Test insert string", "[vector]") {
    prt::vector<std::string> vec;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec.push_back(str);
    }
    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test copy constructor", "[vector]") {
    prt::vector<std::string> vec1;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }
    prt::vector<std::string> vec2 = vec1;
    REQUIRE(vec2.size() == vec1.size());

    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec2[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test copy assignment operator", "[vector]") {
    prt::vector<std::string> vec1, vec2;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }
    vec2 = vec1;
    REQUIRE(vec2.size() == vec1.size());

    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec2[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test nested", "[vector]") {
    prt::vector<prt::vector<uint32_t>> test;
    test.resize(100);
    for (uint32_t i = 0; i <  test.size(); i++) {
        test[i].resize(100);
        for  (uint32_t j = 0; j < test[i].size(); j++) {
            test[i][j] = i * j + i + j;
        }
    }
    for (uint32_t i = 0; i <  test.size(); i++) {
        for  (uint32_t j = 0; j < test[i].size(); j++) {
            REQUIRE(test[i][j] == i * j + i + j);
        }
    }
}