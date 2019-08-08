#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/array.h"


TEST_CASE( "Test array", "[hash_table]") {
    prt::array<uint32_t, 5> arr = {0, 1, 2, 3, 4};

    for (size_t i = 0; i < 5; i++) {
        REQUIRE(arr[i] == i);
    }

}