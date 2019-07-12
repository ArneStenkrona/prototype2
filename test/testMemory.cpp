#include "test/testMemory.h"
#include "src/memory/stack_allocator.h"

TEST_CASE( "Test allocation", "[stack_allocator]" ) {
    uint32_t n = 100;
    // We need to allocated memory for n elements + n bytes for
    // alignment metadata
    size_t bytes = n * sizeof(uint32_t) + n;
    StackAllocator allocator = StackAllocator(reinterpret_cast<uintptr_t>(malloc(bytes)), bytes);

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

TEST_CASE( "Test alignment", "[stack_allocator]" ) {
    // TODO: IMPLEMENT
    REQUIRE( 0 == 1);
}

TEST_CASE( "Test free memory", "[stack_allocator]" ) {
    // TODO: IMPLEMENT
    REQUIRE( 0 == 1);
}

TEST_CASE( "Test reuse memory", "[stack_allocator]" ) {
    // TODO: IMPLEMENT
    REQUIRE( 0 == 1);
}