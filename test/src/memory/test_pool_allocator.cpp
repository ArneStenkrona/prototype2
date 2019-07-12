#include "test/src/prt_test.h"
#include "src/memory/pool_allocator.h"
#include <catch2/catch.hpp>


TEST_CASE( "Test pool allocation", "[pool_allocator]" ) {
    size_t bytes = 1000;
    uintptr_t memoryPointer = reinterpret_cast<uintptr_t>(malloc(bytes));

    size_t blocksize = 4 * sizeof(uint32_t);
    size_t alignment = sizeof(uint32_t);

    PoolAllocator allocator = PoolAllocator(memoryPointer, bytes, blocksize, alignment);

    size_t numBlocks = allocator.getNumberOfBlocks();

    for (uint32_t i = 0; i < numBlocks; i++) {
        allocator.allocate();
    }
}