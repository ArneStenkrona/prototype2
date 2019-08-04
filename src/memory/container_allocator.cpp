#include "container_allocator.h"
#include "src/config/prototype2Config.h"

#include "memory_util.h"

#include <string.h>

#include <iostream>

size_t prt::ContainerAllocator::calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment) {
    assert(alignment <= blockSize);
    assert(blockSize <= memorySizeBytes);

    size_t padding = prt::memory_util::calcPadding(memoryPointer, alignment);

    size_t effectiveMemorySize = reinterpret_cast<uintptr_t>(memorySizeBytes) -
                                    padding;
    
    size_t numBlocks = effectiveMemorySize / (blockSize + sizeof(size_t));
    return numBlocks;
}

prt::ContainerAllocator prt::ContainerAllocator::defaultContainerAllocator = 
                        prt::ContainerAllocator(malloc(DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES),
                                                DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES,
                                                DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES,
                                                DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES);

prt::ContainerAllocator::ContainerAllocator(void* memoryPointer, size_t memorySizeBytes,
                        size_t blockSize, size_t alignment)
                        : _memoryPointer(memoryPointer),
                          _blockSize(blockSize),
                          _alignment(alignment),
                          _numBlocks(calcNumBlocks(reinterpret_cast<uintptr_t>(memoryPointer),
                                                   memorySizeBytes, blockSize, alignment)),
                          _initialPadding(prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(memoryPointer),
                                                      alignment)),
                          _numFreeBlocks(_numBlocks) {
    assert(alignment > 0);
    assert(alignment <= blockSize);
    assert(alignment <= 128);
    assert(blockSize % alignment == 0);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2

    uintptr_t memPtr = reinterpret_cast<uintptr_t>(memoryPointer);
    _blocks = reinterpret_cast<size_t*>(memPtr + memorySizeBytes - (_numBlocks * sizeof(size_t)));

    for (unsigned int i = 0; i < _numBlocks; i++) {
        _blocks[i] = 0;
    }
}

void* prt::ContainerAllocator::allocate(size_t sizeBytes, size_t alignment) {
    assert(alignment <= _blockSize);
    assert(alignment >= 1);
    assert(alignment <= 128);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2

    // Make sure to round up.
    size_t blocks =  alignment <= _alignment ? 
        (sizeBytes + (_blockSize - 1)) / _blockSize :
        ((sizeBytes + alignment) + (_blockSize - 1)) / _blockSize;

    uintptr_t blockPointer = reinterpret_cast<uintptr_t>(allocate(blocks));
    size_t padding = prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(blockPointer),
                                 alignment);
    void* mem = reinterpret_cast<void*>(blockPointer + padding);

    return mem;
}

void prt::ContainerAllocator::free(void* pointer) {
    size_t blockIndex = pointerToBlockGroupIndex(pointer);

    _numFreeBlocks += _blocks[blockIndex];

    _blocks[blockIndex] = 0;
}

void prt::ContainerAllocator::clear() {
    for (unsigned int i = 0; i < _numBlocks; i++) {
        _blocks[i] = 0;
    }
    _numFreeBlocks = _numBlocks;
}

void* prt::ContainerAllocator::allocate(size_t blocks) {
    assert(blocks > 0);
    assert(blocks <= _numBlocks);
    assert(_numFreeBlocks >= blocks);
    // Find suitable range of blocks. O(n)
    size_t blockIndex = 0;
    size_t blocksInARow = 0;
    do {
        if (_blocks[blockIndex] == 0) {
            blocksInARow++;
            blockIndex++;
        } else {
            blocksInARow = 0;
            blockIndex += _blocks[blockIndex];
        }

        if (blocksInARow == blocks) {
            _blocks[blockIndex - blocks] = blocks;

            _numFreeBlocks -= blocks; 
            
            return blockIndexToPointer(blockIndex - blocks);
        }
    } while (blockIndex < _numBlocks);

    assert(false);
    return nullptr;
}