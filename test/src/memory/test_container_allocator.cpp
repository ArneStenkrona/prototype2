#include "test/src/prt_test.h"
#include "src/memory/container_allocator.h"
#include <catch2/catch.hpp>

#include <iostream>
#include <algorithm>
#include <unordered_set>

/* TODO: ADD TESTING FOR ALLOCATIONS LARGER THAN 1 BLOCK */
TEST_CASE( "Test allocation", "[container_allocator]" ) {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t) + sizeof(size_t);

    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;
    
    void* mem = malloc(bytes);
    prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

    size_t numBlocks = allocator.getNumberOfBlocks();

    // array of array of 4 integers
    uint32_t* integers[estimatedNumBlocks];

    for (uint32_t i = 0; i < numBlocks; i++) {
        integers[i] = static_cast<uint32_t*>(allocator.allocate(blocksize - sizeof(size_t), 1));

        for (uint32_t j = 0; j < 4; j++){ 
            integers[i][j] = i * i + j * j;
        }
    }

    REQUIRE(allocator.getNumberOfFreeBlocks() == 0);

    for (uint32_t i = 0; i < numBlocks; i++) {
        for (uint32_t j = 0; j < 4; j++){ 
            REQUIRE(integers[i][j] == i * i + j * j);
        }
    }
    free(mem);
}

TEST_CASE( "Test variable length allocation", "[container_allocator]") {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t);

    void* mem = malloc(bytes);
    prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

    uint32_t* ints1;
    uint32_t* ints2[2]; // array of 2
    uint32_t* ints3[3]; // array of 3
    uint32_t* ints4[4]; // array of 4

    std::unordered_set<uint32_t*> uniquePointers;

    for (size_t i = 0; i < 4; i++) {
        if (i < 1) {
        ints1 = static_cast<uint32_t*>(allocator.allocate(blocksize, 1));
        REQUIRE(uniquePointers.find(ints1) == uniquePointers.end());
        uniquePointers.insert(ints1);
            for (size_t j = 0; j < 4; j++) {
                ints1[j] = i * j + i + j;
            }
        }
        if (i < 2) {
        ints2[i] = static_cast<uint32_t*>(allocator.allocate(blocksize, 1));
        REQUIRE(uniquePointers.find(ints2[i]) == uniquePointers.end());
        uniquePointers.insert(ints2[i]);
            for (size_t j = 0; j < 4; j++) {
                ints2[i][j] = i * j + i + j;
            } 
        }
        if (i < 3) {
        ints3[i] = static_cast<uint32_t*>(allocator.allocate(blocksize, 1));
        REQUIRE(uniquePointers.find(ints3[i]) == uniquePointers.end());
        uniquePointers.insert(ints3[i]);
            for (size_t j = 0; j < 4; j++) {
                ints3[i][j] = i * j + i + j;
            }
        }
        ints4[i] = static_cast<uint32_t*>(allocator.allocate(blocksize, 1));
        REQUIRE(uniquePointers.find(ints4[i]) == uniquePointers.end());
        uniquePointers.insert(ints4[i]);
        for (size_t j = 0; j < 4; j++) {
            ints4[i][j] = i * j + i + j;
        }
    }

    for (size_t i = 0; i < 4; i++) {
        if (i < 1) {
            for (size_t j = 0; j < 4; j++) {
                REQUIRE(ints1[j] == i * j + i + j);
            }
        }
        if (i < 2) {
            for (size_t j = 0; j < 4; j++) {
                REQUIRE(ints2[i][j] == i * j + i + j);
            } 
        }
        if (i < 3) {
            for (size_t j = 0; j < 4; j++) {
                REQUIRE(ints3[i][j] == i * j + i + j);
            }
        }
        for (size_t j = 0; j < 4; j++) {
            REQUIRE(ints4[i][j] == i * j + i + j);
        }
    }
    free(mem);
}

TEST_CASE( "Test alignment", "[container_allocator]" ) {
    size_t powersOfTwo[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
        
    constexpr size_t bytes = 5000;

    constexpr size_t blocksize = 128 + sizeof(size_t);
    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;
    
    void* mem = malloc(bytes);

    for (uint32_t i = 0; i < 8; i++) {
        size_t alignment = powersOfTwo[i];      
        
        prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

        size_t numBlocks = allocator.getNumberOfBlocks();

        // array of array of 4 integers
        uintptr_t pointers[estimatedNumBlocks];
        size_t estBlocksPerAlloc = (128 + sizeof(size_t) + alignment + (blocksize - 1)) / blocksize;
        for (uint32_t j = 0; j < numBlocks / estBlocksPerAlloc; j++) {
            pointers[j] = reinterpret_cast<uintptr_t>(allocator.allocate(blocksize - sizeof(size_t), alignment));
            REQUIRE(pointers[j] % alignment == 0);
        }
    }
    free(mem);
}

TEST_CASE( "Test free memory", "[container_allocator]" ) {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t);

    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;

    void* mem = malloc(bytes);
    prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

    size_t numBlocks = allocator.getNumberOfBlocks();

    void* blocks[estimatedNumBlocks];
    for (uint32_t i = 0; i < numBlocks; i++) {
        blocks[i] = allocator.allocate(1, 1);
    }

    REQUIRE(allocator.getNumberOfFreeBlocks() == 0);

    for (uint32_t i = 0; i < numBlocks; i++) {
        allocator.free(blocks[i]);
        REQUIRE(allocator.getNumberOfFreeBlocks() == i + 1);       
    }
    
    free(mem);
}

TEST_CASE( "Test clear memory", "[container_allocator]" ) {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t);

    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;

    void* mem = malloc(bytes);
    prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

    size_t numBlocks = allocator.getNumberOfBlocks();

    void* blocks[estimatedNumBlocks];

    for (uint32_t i = 0; i < numBlocks; i++) {
        blocks[i] = allocator.allocate(1, 1);
    }

    REQUIRE(allocator.getNumberOfFreeBlocks() == 0);

    allocator.clear();

    REQUIRE(allocator.getNumberOfFreeBlocks() == numBlocks);

    free(mem);
}

TEST_CASE( "Test reuse memory", "[container_allocator]" ) {
    constexpr size_t bytes = 1000;

    constexpr size_t blocksize = 4 * sizeof(uint32_t) + 2 * sizeof(size_t);

    // number of blocks without considering allocator padding
    constexpr size_t estimatedNumBlocks = bytes / blocksize;

    void* mem = malloc(bytes);
    prt::ContainerAllocator allocator = prt::ContainerAllocator(mem, bytes, blocksize);

    size_t numBlocks = allocator.getNumberOfBlocks();

    // array of array of 4 integers
    uint32_t* integers[estimatedNumBlocks];

    for (uint32_t i = 0; i < numBlocks; i++) {
        integers[i] = static_cast<uint32_t*>(allocator.allocate(blocksize - sizeof(size_t), 1));

        for (uint32_t j = 0; j < 4; j++){ 
            integers[i][j] = i * i + j * j;
        }
    }

    for (uint32_t i = 0; i < numBlocks; i++) {
        for (uint32_t j = 0; j < 4; j++){ 
            REQUIRE(integers[i][j] == i * i + j * j);
        }
    }

    allocator.clear();

    for (uint32_t i = 0; i < numBlocks; i++) {
        integers[i] = static_cast<uint32_t*>(allocator.allocate(blocksize - sizeof(size_t), 1));

        for (uint32_t j = 0; j < 4; j++){ 
            integers[i][j] = i * i * i + j * j * j;
        }
    }

    for (uint32_t i = 0; i < numBlocks; i++) {
        for (uint32_t j = 0; j < 4; j++){ 
            REQUIRE(integers[i][j] == i * i * i + j * j * j);
        }
    }
    free(mem);
}