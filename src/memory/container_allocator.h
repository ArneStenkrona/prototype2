#ifndef CONTAINER_ALLOCATOR_H
#define CONTAINER_ALLOCATOR_H

#include  <stddef.h>

#include <assert.h>

namespace prt {
    class ContainerAllocator {
    public:
        explicit ContainerAllocator() = delete;

        /** 
         * Constructs a container allocator with the given total size.
         * The caller is responsible for ensuring that memoryPointer
         * points to a valid, free memory block with size of
         * memorySizeBytes
         * 
         * The allocator will be aligned with alignment such that each 
         * block begins on an aligned address. 
         * 
         * @param memoryPointer pointer to the first memory address
         *        of the allocator
         * @param memorySizeBytes size of memory in bytes
         * @param blocksize size of block in bytes
         * @param alignment aligment in bytes
         */
        explicit ContainerAllocator(void* memoryPointer, size_t memorySizeBytes,
                                    size_t blockSize, size_t alignment);

        /**
         * Allocates contiguous memory from the allocator
         * with specified alignment. 
         * 
         * Internally a suitable continuous group of blocks
         * are found that satisfies the size request.
         * 
         * @param sizeBytes size of memory in bytes
         * @param alignment alignment in bytes
         * 
         * @return pointer to allocated memory
         */
        void* allocate(size_t sizeBytes, size_t alignment);

        /**
         * Frees memory at pointer address
         * 
         * The pointer is expected to be within the allocators
         * address space.
         * 
         * Internally, the allocator looks for the block group
         * that contains the address and frees that group
         * 
         * @param pointer pointer to address
         */
        void free(void* pointer);

        /**
         * Clears all memory within the allocator
         */
        void clear();

        inline size_t getAlignment() const { return _alignment; }

        inline size_t getNumberOfBlocks() const { return _numBlocks; }

        inline size_t getNumberOfFreeBlocks() const { return _numFreeBlocks; }

        /**
         * @return default container allocator
         */
        static ContainerAllocator& getDefaultContainerAllocator();

    private:
        void* allocate(size_t blocks);

        size_t calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment);

        inline void* blockIndexToPointer(size_t blockIndex) const {
            uintptr_t pointer = reinterpret_cast<uintptr_t>(_memoryPointer) + 
                                _initialPadding +
                                (blockIndex * _blockSize);
            return reinterpret_cast<void*>(pointer);
        }

        inline size_t pointerToBlockIndex(void* pointer) const {
            uintptr_t ptr = reinterpret_cast<uintptr_t>(pointer);
            uintptr_t memStart = reinterpret_cast<uintptr_t>(_memoryPointer) + 
                                 _initialPadding;

            assert(ptr >= memStart);

            uintptr_t diff = ptr - memStart;
            
            return diff / _blockSize;
        }

        inline size_t pointerToBlockGroupIndex(void* pointer) const {
            size_t blockIndex = pointerToBlockIndex(pointer);
            while (_blocks[blockIndex] == 0 && blockIndex > 0) {
                blockIndex--;
            }
            return blockIndex;
        }

        void* _memoryPointer;

        // Size of block.
        size_t _blockSize;
        // Data alignment.
        size_t _alignment;
        // Number of blocks.
        size_t _numBlocks;
        // Padding at the start of the memory
        size_t _initialPadding;
        // Number of free blocks
        size_t _numFreeBlocks;

        /**
         * A block group is a contiguous set of blocks
         * allocated together.
         * 
         * _blocks[0] is always the start of a block
         * group. 
         * 
         * If _blocks[i] is the start of a block group
         * and zero it indicates a free block group
         * 
         * If _blocks[i] is the start of a block group
         * and non-zero it indicates the distance to
         * the next block group.
         * e.g. _blocks[i] is 3 indicates that the 
         * next block group starts at _blocks[i + 3] 
         * 
         * If _blocks[i] is not the start of a block
         * group its value is zero
         */
        size_t* _blocks;
    };
}

#endif