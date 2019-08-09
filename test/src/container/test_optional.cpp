#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/optional.h"

TEST_CASE( "Test optional", "[hash_table]") {
    prt::optional<uint32_t> opt1;
    REQUIRE(!opt1.has_value());
    REQUIRE(opt1.value_or(16) == 16);
    opt1 = 13;
    REQUIRE(opt1.has_value());
    REQUIRE(opt1.value() == 13);
    REQUIRE(opt1.value_or(16) == 13);
    opt1.reset();
    REQUIRE(!opt1.has_value());
}

TEST_CASE( "Test compare operators", "[hash_table]") {
}