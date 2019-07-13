#include "test/src/prt_test.h"
#include "src/memory/pool_allocator.h"
#include <catch2/catch.hpp>

TEST_CASE( "Test pool allocation", "[pool_allocator]" ) {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t);
    constexpr size_t alignment = sizeof(uint32_t);

    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;

    PoolAllocator allocator = PoolAllocator(malloc(bytes), bytes, blocksize, alignment);

    size_t numBlocks = allocator.getNumberOfBlocks();

    // array of array of 4 integers
    uint32_t* integers[estimatedNumBlocks];

    for (uint32_t i = 0; i < numBlocks; i++) {
        integers[i] = static_cast<uint32_t*>(allocator.allocate());

        for (uint32_t j = 0; j < 4; j++){ 
            integers[i][j] = i * i + j * j;
        }
    }

      for (uint32_t i = 0; i < numBlocks; i++) {
        for (uint32_t j = 0; j < 4; j++){ 
            REQUIRE(integers[i][j] == i * i + j * j);
        }
    }
}

TEST_CASE( "Test pool alignment", "[pool_allocator]" ) {
    size_t powersOfTwo[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    for (uint32_t i = 0; i < 8; i++) {
        constexpr size_t bytes = 5000;

        constexpr size_t blocksize = 128;
        size_t alignment = powersOfTwo[i];

        // number of blocks without considering allocator padding
        constexpr size_t estimatedNumBlocks = bytes / blocksize;

        PoolAllocator allocator = PoolAllocator(malloc(bytes), bytes, blocksize, alignment);

        size_t numBlocks = allocator.getNumberOfBlocks();

        // array of array of 4 integers
        uintptr_t pointers[estimatedNumBlocks];

        for (uint32_t j = 0; j < numBlocks; j++) {
            pointers[j] = reinterpret_cast<uintptr_t>(allocator.allocate());
            REQUIRE(pointers[j] % alignment == 0);
        }
    }
}