#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/hash_table.h"

TEST_CASE( "Test hash table", "[hash_table]") {
    prt::HashTable<uint32_t, uint32_t> table;

    table.insert(5, 4);

    REQUIRE(table.find(5)->value == 4);
}