#include "pool_allocator.h"

#include "memory_util.h"

/**
 * Helper function for constructor.
 * Calculates the number of blocks given the
 * arguments to the pool allocator constructor.
 */
size_t calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment) {
    assert(alignment <= blockSize);
    assert(blockSize <= memorySizeBytes);

    size_t padding = prt::memory_util::calcPadding(memoryPointer, alignment);

    size_t effectiveBlockSize = blockSize + prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(blockSize),
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
                              _initialPadding(prt::memory_util::calcPadding(_memoryPointer, alignment)),
                              _blockPadding(prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(blockSize),
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
        _freeQueueHead = _memoryPointer + _initialPadding;
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
