#include "test/src/prt_test.h"

#include "src/memory/stack_allocator.h"

#include <catch2/catch.hpp>


TEST_CASE( "Test stack allocation", "[stack_allocator]" ) {
    uint32_t n = 100;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = 
        StackAllocator(malloc(bytes), bytes);

    uint32_t* integers = static_cast<uint32_t*>
                    (allocator
                    .allocate(n * sizeof(uint32_t), sizeof(uint32_t)));

    for (uint32_t i = 0; i < n; i++){
        integers[i] = i * i;
    }
    for (uint32_t i = 0; i < n; i++) {
        REQUIRE(integers[i] == i * i);
    }
}

TEST_CASE( "Test stack alignment", "[stack_allocator]" ) {
    
    uint32_t n = 1000;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = 
        StackAllocator(malloc(bytes), bytes);

    size_t powersOfTwo[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    
    for (uint32_t i = 0; i < 100; i ++) {
        size_t alignment = powersOfTwo[i % 8];
        uintptr_t pointer = reinterpret_cast<uintptr_t>
                        (allocator
                        .allocate(sizeof(uint32_t), alignment));
        REQUIRE(pointer % alignment == 0);
    }
}

TEST_CASE( "Test free stack memory", "[stack_allocator]" ) {
    uint32_t n = 100;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = 
        StackAllocator(malloc(bytes), bytes);

    size_t alignment = 1;
    uint32_t* integers = static_cast<uint32_t*>
                    (allocator
                    .allocate(n * sizeof(uint32_t), alignment));

    for (uint32_t i = 0; i < n; i++){
        integers[i] = i * i;
    }
    for (uint32_t i = n - 1; i >= 1; i--) {        
        uint32_t* freeAddress = &integers[i];
        allocator.free(reinterpret_cast<void*>(freeAddress));
        REQUIRE(allocator.getMarker() == i * sizeof(uint32_t) + (alignment));
    }
    // When freeing the last integer the alignment padding
    // is gone, so we compare this with new logic.
    allocator.free(reinterpret_cast<void*>(integers));
    REQUIRE(allocator.getMarker() == 0);
}

TEST_CASE( "Test clear stack memory", "[stack_allocator]" ) {
    uint32_t n = 100;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = 
        StackAllocator(malloc(bytes), bytes);

    uint32_t* integers = static_cast<uint32_t*>
                    (allocator
                    .allocate(n * sizeof(uint32_t), sizeof(uint32_t)));

    allocator.clear();
    REQUIRE(allocator.getMarker() == 0);
}

TEST_CASE( "Test reuse stack memory", "[stack_allocator]" ) {
    uint32_t n = 100;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = 
        StackAllocator(malloc(bytes), bytes);

   uint32_t* integers = static_cast<uint32_t*>
                    (allocator
                    .allocate(n * sizeof(uint32_t), sizeof(uint32_t)));

    for (uint32_t i = 0; i < n; i++){
        integers[i] = i * i;
    }
    allocator.clear();
    integers = static_cast<uint32_t*>
                    (allocator
                    .allocate(n * sizeof(uint32_t), sizeof(uint32_t)));

    for (uint32_t i = 0; i < n; i++){
        integers[i] = i * i * i;
    }

    for (uint32_t i = 0; i < n; i++) {
        REQUIRE(integers[i] == i * i * i);
    }
}