#include "container_allocator.h"
#include "src/config/prototype2Config.h"

#include "memory_util.h"

#include <string.h>

#include <iostream>

#include <algorithm>

#include <new>

prt::ALIGNMENT prt::getAlignment(size_t alignment) {
        switch(alignment) {
            case 1      : return ALIGNMENT::ALIGN_1_BYTE;
            case 2      : return ALIGNMENT::ALIGN_2_BYTES;
            case 4      : return ALIGNMENT::ALIGN_4_BYTES;
            case 8      : return ALIGNMENT::ALIGN_8_BYTES;
            case 16     : return ALIGNMENT::ALIGN_16_BYTES;
            case 32     : return ALIGNMENT::ALIGN_32_BYTES;
            case 64     : return ALIGNMENT::ALIGN_64_BYTES;
            case 128    : return ALIGNMENT::ALIGN_128_BYTES;
            case 256    : return ALIGNMENT::ALIGN_256_BYTES;
            // case 512    : return ALIGNMENT::ALIGN_512_BYTES;
            // case 1024   : return ALIGNMENT::ALIGN_1024_BYTES;
        }
        assert(false && "Invalid alignment. Alignment must be a power of 2. ");
        return ALIGNMENT::ALIGN_1_BYTE;
    }

alignas(prt::ContainerAllocator) static char defaultContainerAllocatorBuffer[sizeof(prt::ContainerAllocator)];
alignas(std::max_align_t)        static char defaultContainerAllocatorMemory[DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES];

prt::ContainerAllocator& prt::ContainerAllocator::getDefaultContainerAllocator() {
    static prt::ContainerAllocator* defaultContainerAllocator = 
        new (&defaultContainerAllocatorBuffer) prt::ContainerAllocator(static_cast<void*>(defaultContainerAllocatorMemory),
                                    DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES,
                                    DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES,
                                    DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES);
    return *defaultContainerAllocator;
}

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

prt::ContainerAllocator::ContainerAllocator(void* memoryPointer, size_t memorySizeBytes,
                        size_t blockSize, size_t alignment)
                        : _memoryPointer(memoryPointer),
                          _blockSize(blockSize),
                          _alignment(alignment),
                          _numBlocks(calcNumBlocks(reinterpret_cast<uintptr_t>(memoryPointer),
                                                   memorySizeBytes, blockSize, alignment)),
                          _initialPadding(prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(memoryPointer),
                                                                        alignment)),
                          _numFreeBlocks(_numBlocks),
                          _firstFreeBlockIndex(0) {
    assert(alignment > 0);
    assert(alignment <= blockSize);
    // assert(alignment <= 128);
    assert(alignment <= 256);
    assert(blockSize % alignment == 0);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2

    uintptr_t memPtr = reinterpret_cast<uintptr_t>(memoryPointer) + _initialPadding;
    _blocks = reinterpret_cast<size_t*>(memPtr + memorySizeBytes - (_numBlocks * sizeof(size_t)));

    for (unsigned int i = 0; i < _numBlocks; i++) {
        _blocks[i] = 0;
    }
}

void* prt::ContainerAllocator::allocate(size_t sizeBytes, size_t alignment) {
    assert(alignment <= _blockSize);
    assert(alignment >= 1);
    // assert(alignment <= 128);
    assert(alignment <= 256);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2
    assert((sizeBytes <= _numFreeBlocks * _blockSize));

    // Make sure to round up.
    size_t blocks = alignment <= _alignment ? 
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

    if (blockIndex < _firstFreeBlockIndex) {
        _firstFreeBlockIndex = blockIndex;
    }
}

void prt::ContainerAllocator::clear() {
    for (unsigned int i = 0; i < _numBlocks; i++) {
        _blocks[i] = 0;
    }
    _numFreeBlocks = _numBlocks;
    _firstFreeBlockIndex = 0;
}

/*void* prt::ContainerAllocator::allocate(size_t blocks) {
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
}*/

void* prt::ContainerAllocator::allocate(size_t blocks) {
    assert(blocks > 0);
    assert(blocks <= _numBlocks);
    assert(_numFreeBlocks >= blocks);
    assert(_firstFreeBlockIndex < _numBlocks);
    // Find suitable range of blocks. O(n)
    size_t blockIndex = _firstFreeBlockIndex;
    size_t blocksInARow = 0;
    size_t it = 0;
    do {
        ++it;
        if (_blocks[blockIndex] == 0) {
            blocksInARow++;
            blockIndex++;
        } else {
            blocksInARow = 0;
            blockIndex += _blocks[blockIndex];
        }

        if (blocksInARow == blocks) {
            size_t index = blockIndex - blocks;
            _blocks[index] = blocks;

            _numFreeBlocks -= blocks; 

            if (index == _firstFreeBlockIndex) {
                do {
                    _firstFreeBlockIndex += _blocks[_firstFreeBlockIndex];
                } while(_firstFreeBlockIndex < _numBlocks && _blocks[_firstFreeBlockIndex] != 0);
            }
            std::cout << "it: " << it << std::endl; 
            return blockIndexToPointer(index);
        }
    } while (blockIndex < _numBlocks);

    for (size_t i = 0; i < _numBlocks; ++i) {
        if (_blocks[i] == 0) {
            assert((i >= _firstFreeBlockIndex) && "ERROEREROR!");
        }
    }

    assert(false);
    return nullptr;
}