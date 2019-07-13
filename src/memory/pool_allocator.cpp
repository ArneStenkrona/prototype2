#include "pool_allocator.h"

#include <cassert>

/**
 * Helper function for constructor.
 * Calculates padding given a memory pointer
 * and desired alignment.
 */ 
size_t calcPadding(uintptr_t memoryPointer, size_t alignment){
    assert(alignment >= 1);
    assert(alignment <= 128);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2

    // cast so we can perform arithmetic on stack pointer
    //uintptr_t sPtr = reinterpret_cast<uintptr_t>(_memoryPointer);

    // Allocate unaligned block & convert address to uintptr_t.
    uintptr_t rawAdress = reinterpret_cast<uintptr_t>(memoryPointer);//reinterpret_cast<uintptr_t>(sPtr + _stackMarker);

    // Calculate the adjustment by masking off the lower bits
    // of the address, to determine how "misaligned" it is.
    size_t mask = (alignment - 1);
    size_t misalignment = (rawAdress & mask);
    // We don't need to store any meta-information
    //size_t adjustment = alignment - misalignment;

    return misalignment;
}

/**
 * Helper function for constructor.
 * Calculates the number of blocks given the
 * arguments to the pool allocator constructor.
 */
size_t calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment) {
    assert(alignment <= blockSize);
    assert(blockSize <= memorySizeBytes);

    size_t padding = calcPadding(memoryPointer, alignment);

    size_t effectiveBlockSize = blockSize + calcPadding(reinterpret_cast<uintptr_t>(blockSize),
                                            alignment);

    size_t effectiveMemorySize = reinterpret_cast<uintptr_t>(memorySizeBytes) -
                                    padding;
    
    size_t numBlocks = effectiveMemorySize / effectiveBlockSize;
    return numBlocks;
}

PoolAllocator::PoolAllocator(void* memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment)
                            : Allocator(memoryPointer, memorySizeBytes),
                              _blockSize(blockSize), _alignment(alignment),
                              _numBlocks(calcNumBlocks(_memoryPointer, _memorySizeBytes,
                                                       _blockSize, _alignment)),
                              _initialPadding(calcPadding(_memoryPointer, alignment)),
                              _blockPadding(calcPadding(reinterpret_cast<uintptr_t>(blockSize),
                                                        alignment)),
                              _numFreeBlocks(_numBlocks),
                              _freeQueueHead(_memoryPointer + _initialPadding) {
    initFreeBlockQueue();
}

    void* PoolAllocator::allocate() {
        // Make sure _freeQueue is not empty.
        assert(_freeQueueHead != reinterpret_cast<uintptr_t>(nullptr));
        // Cast to void*
        void* allocatedPointer = reinterpret_cast<void*>( _freeQueueHead);
        // Set head of queue to next link.
        _freeQueueHead = *reinterpret_cast<uintptr_t*>(_freeQueueHead);
        // Decrement number of free blocks.
        _numFreeBlocks--;

        return allocatedPointer;
    }

    void PoolAllocator::free(void* pointer) {
        // Make sure the pointer is a valid pointer
        // To a block in the pool
        assert((reinterpret_cast<size_t>(pointer) -
                (reinterpret_cast<size_t>(_memoryPointer) + _initialPadding)) 
                % (_blockSize + _blockPadding) == 0);
        assert(reinterpret_cast<size_t>(pointer) >=
                reinterpret_cast<size_t>(_memoryPointer));
        assert(reinterpret_cast<size_t>(pointer) <=
                reinterpret_cast<size_t>(_memoryPointer) + _memorySizeBytes);

        uintptr_t* freePointer = reinterpret_cast<uintptr_t*>(pointer);
        *freePointer = _freeQueueHead;
        _freeQueueHead = reinterpret_cast<uintptr_t>(pointer);

        // Increment number of free blocks.
        _numFreeBlocks++;
    }

    void PoolAllocator::initFreeBlockQueue() {
        // Insert all blocks into free Queue
        uintptr_t currentElement = _freeQueueHead;
        for (size_t i = 0; i < _numBlocks - 1; i++) {
            // Next element is an offset of block size and block padding
            // away from current
            uintptr_t nextElement = currentElement + _blockSize + _blockPadding;
            uintptr_t* curr = reinterpret_cast<uintptr_t*>(currentElement);
            *curr = nextElement;
            currentElement = nextElement;
        }
        // Last elemet is nullptr.
        uintptr_t* curr = reinterpret_cast<uintptr_t*>(currentElement);
        *curr = reinterpret_cast<uintptr_t>(nullptr);

        _numFreeBlocks = _numBlocks;
    }
