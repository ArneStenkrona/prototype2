#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/hash_table.h"

#include <string>

TEST_CASE( "Test insert", "[hash_table]") {
    prt::HashTable<uint32_t, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        table.insert(i, i * i - i);
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "Test remove", "[hash_table]") {
    prt::HashTable<uint32_t, uint32_t> table;

    size_t s = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        table.insert(i, i * i - i);
        s++;
    }

    for (uint32_t i = 0; i < 1000; i++) {
        if (i % 3 == 0) {
            table.remove(i);
            s--;
        }
    }

    for (uint32_t i = 0; i < 1000; i++) {
        if (i % 3 == 0) {
            REQUIRE((table.find(i) == table.end()));
        } else {
            REQUIRE((table.find(i) != table.end()));
        }
    }
    REQUIRE(table.size() == s);
}

TEST_CASE( "Test subscript", "[hash_table]") {
    prt::HashTable<uint32_t, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        table[i] = i * i - i;
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "Test find", "[hash_table]") {
    prt::HashTable<std::string, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        table[str] = i;
    }

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((table.find(str) != table.end()));
    }
    for (uint32_t i = 1000; i < 2000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((table.find(str) == table.end()));
    }
}

TEST_CASE( "Test iterate", "[hash_table]") {
    prt::HashTable<std::string, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        table[str] = i;
    }

    for (auto it = table.begin(); it != table.end(); it++) {
        std::string& key = it->key();
        uint32_t val = it->value();
        REQUIRE(key == std::to_string(val));
    }
}