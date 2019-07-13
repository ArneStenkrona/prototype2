#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include "allocator.h"
#include <cstdint>

/**
 * Implementation of a pool allocator
 * 
 * TODO: write more thorough description
 * 
 * The allocator keeps a queue containing the next free block to
 * be allocated as the head.
 * The queue is implemented as a single linked
 * list. 
 * The head of the linked list is stored as a member variable.
 * A free block's next pointer is stored in that free block.
 * 
 * Whenever a block is allocated it is popped from the queue by
 * setting the head of the queue to the blocks next pointer
 * 
 * Whenever a block is freed it is pushed onto the queue by
 * setting the head of the queue to the blocks address
 */
class PoolAllocator : public Allocator {
public:
    /**
     * Constructs a pool allocator with the given total size.
     * The caller is responsible for ensuring that memoryPointer
     * points to a valid, free memory block with size of
     * memorySizeBytes
     * 
     * The pool will be aligned with alignment such that each 
     * block begins on an aligned address. 
     * This will lead to padding at the end of blocks if block 
     * size is not a multiple of the alignment
     * 
     * @param memoryPointer pointer to the first memory address
     *        of the allocator
     * @param memorySizeBytes size of memory in bytes
     * @param alignment aligment in bytes
     */
    explicit PoolAllocator(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment);

    /**
     * Returns a pointer to a block with size defined by the 
     * allocator instance
     * It is the callers responsibility to stay within this 
     * block
     * 
     * @return pointer to allocated block
     */
    void* allocate();
    void free(void* pointer) override;

    inline size_t getBlockSize() const { return _blockSize; }
    inline size_t getNumberOfBlocks() const { return _numBlocks; }

private:
    // Size of block.
    const size_t _blockSize;
    // Data alignment.
    const size_t _alignment;
    // Number of blocks.
    const size_t _numBlocks;
    // Padding at the start of the memory
    const size_t _initialPadding;
    // Padding required for block alignment
    const size_t _blockPadding;

    // Pointer to head of queue containing
    // free blocks
    uintptr_t _freeQueueHead;

    // This method does not apply to pool allocator
    // and is hidden.
    void* allocate(size_t, size_t) override 
        { return reinterpret_cast<void*>(0);};

};

#endif