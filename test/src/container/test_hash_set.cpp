#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/hash_set.h"

#include <string>

TEST_CASE( "hash_set: Test insert", "[hash_set]") {
    prt::hash_set<std::string> set;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        set.insert(str);
    }
    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((set.find(str) != set.end()));
    }
}

TEST_CASE( "hash_set: Test find", "[hash_set]") {
    prt::hash_set<std::string> set;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        set.insert(str);
    }

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((set.find(str) != set.end()));
    }
    for (uint32_t i = 1000; i < 2000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((set.find(str) == set.end()));
    }
}

TEST_CASE( "hash_set: Test iterate", "[hash_set]") {
    prt::hash_set<std::string> set;

    for (uint32_t i = 0; i < 1000; i++) {
        set.insert("hello");
    }

    for (auto it = set.begin(); it != set.end(); it++) {
        std::string val = it->value();
        REQUIRE(val == "hello");
    }
}