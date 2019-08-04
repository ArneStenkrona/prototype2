#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/hash_table.h"

TEST_CASE( "Test hash table", "[hash_table]") {
    prt::HashTable<uint32_t, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        table.insert(i, i * i - i);
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table.find(i)->value == i * i - i);
    }
    prt::HashTable<uint32_t, uint32_t> table2;

    for (uint32_t i = 0; i < 1000; i++) {
        table2[i] = i * i - i;
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table2.find(i)->value == i * i - i);
    }
}