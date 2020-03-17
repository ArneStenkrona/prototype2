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
                                    DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES/*,
                                    DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES*/);
    return *defaultContainerAllocator;
}

size_t prt::ContainerAllocator::calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment) {
    assert(alignment <= blockSize);
    assert(blockSize <= memorySizeBytes);

    size_t padding = prt::memory_util::calcPadding(memoryPointer, alignment);

    size_t effectiveMemorySize = reinterpret_cast<uintptr_t>(memorySizeBytes) -
                                    padding;
    
    size_t numBlocks = effectiveMemorySize / (blockSize /*+ sizeof(size_t)*/);
    return numBlocks;
}

prt::ContainerAllocator::ContainerAllocator(void* memoryPointer, size_t memorySizeBytes,
                        size_t blockSize/*, size_t alignment*/)
                        : _memoryPointer(memoryPointer),
                          _paddedMemoryPointer(nullptr),
                          _blockSize(blockSize),
                          /*_alignment(alignment),*/
                          _numBlocks(calcNumBlocks(reinterpret_cast<uintptr_t>(memoryPointer),
                                                   memorySizeBytes, blockSize, _alignment)),
                          _initialPadding(prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(memoryPointer),
                                                                        _alignment)),
                          _numFreeBlocks(_numBlocks),
                          _firstFreeBlockIndex(0) {
    assert(_alignment > 0);
    assert(_blockSize >= sizeof(size_t));
    assert(_alignment <= _blockSize);
    // assert(alignment <= 128);
    assert(_alignment <= 256);
    assert(_blockSize % _alignment == 0);
    assert((_alignment & (_alignment - 1)) == 0); // verify power of 2

    uintptr_t memPtr = reinterpret_cast<uintptr_t>(memoryPointer) + _initialPadding;
    _paddedMemoryPointer = reinterpret_cast<void*>(memPtr);

    for (size_t i = 0; i < _numBlocks; ++i) {
        *reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>(_paddedMemoryPointer)[i * _blockSize])) = i + 1;
    } 
}

void* prt::ContainerAllocator::allocate(size_t sizeBytes, size_t alignment) {
    assert(alignment + sizeof(size_t) <= _blockSize);
    assert(alignment >= 1);
    // assert(alignment <= 128);
    assert(alignment <= 256);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2
    assert((sizeBytes <= _numFreeBlocks * _blockSize));

    // allocated enough to store number of blocks + sizeBytes + padding
    size_t blocks = alignment <= _alignment ? 
                    (sizeof(size_t) + sizeBytes + _blockSize - 1) / _blockSize :
                    (sizeof(size_t) + sizeBytes + alignment + _blockSize - 1) / _blockSize;
    
    uintptr_t blockPointer = reinterpret_cast<uintptr_t>(allocate(blocks));
    size_t padding = prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(blockPointer + sizeof(size_t)),
                                                   alignment);
    *reinterpret_cast<size_t*>(blockPointer) = blocks;                                          
    void *mem = reinterpret_cast<void*>(blockPointer + sizeof(size_t) + padding);
    return mem;
}

void prt::ContainerAllocator::free(void* pointer) {

    size_t blockIndex = pointerToBlockIndex(pointer);
    void *mem = blockIndexToPointer(blockIndex);
    _numFreeBlocks += *reinterpret_cast<size_t*>(mem);
    
    size_t *prev = &_firstFreeBlockIndex;

    while (*prev <  blockIndex) {
        prev = reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>(_paddedMemoryPointer)[*prev * _blockSize]));
    };
    *reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>(_paddedMemoryPointer)[blockIndex * _blockSize])) = *prev;
    *prev = blockIndex;
}

void prt::ContainerAllocator::clear() {
    _numFreeBlocks = _numBlocks;
    _firstFreeBlockIndex = 0;
    for (size_t i = 0; i < _numBlocks; ++i) {
        *reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>(_paddedMemoryPointer)[i * _blockSize])) = i + 1;
    } 
}

void* prt::ContainerAllocator::allocate(size_t blocks) {
    assert(blocks > 0);
    assert(blocks <= _numBlocks);
    assert(_numFreeBlocks >= blocks);
    assert(_firstFreeBlockIndex < _numBlocks);
    // Find suitable range of blocks. O(n)
    size_t *blockIndex = &_firstFreeBlockIndex;
    size_t index = *blockIndex;
    size_t blocksInARow = 0;

    size_t *prev = &_firstFreeBlockIndex;

    do {
        // Find the next block index from the linked list stored in free blocks
        size_t *nextIndex = reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>(_paddedMemoryPointer)[*blockIndex * _blockSize]));
        if (*nextIndex - *blockIndex == 1) {
            ++blocksInARow;
        } else {
            blocksInARow = 0;
            prev = blockIndex;
            index = *nextIndex;
        }
        // Update index
        blockIndex = nextIndex;

        if (blocksInARow == blocks) {
            _numFreeBlocks -= blocks;

            size_t nextFreeIndex = *blockIndex;
            *prev = nextFreeIndex;
            return blockIndexToPointer(index);
        }

    } while (*blockIndex < _numBlocks);

    assert(false);
    return nullptr;
}