#ifndef CONTAINER_ALLOCATOR_H
#define CONTAINER_ALLOCATOR_H

#include  <stddef.h>

#include <assert.h>

#include <iostream>

namespace prt {
    enum class ALIGNMENT : size_t {
        ALIGN_1_BYTE      = 1,
        ALIGN_2_BYTES     = 2,
        ALIGN_4_BYTES     = 4,
        ALIGN_8_BYTES     = 8,
        ALIGN_16_BYTES    = 16,
        ALIGN_32_BYTES    = 32,
        ALIGN_64_BYTES    = 64,
        ALIGN_128_BYTES   = 128,
        ALIGN_256_BYTES   = 256,
        // ALIGN_512_BYTES   = 512,
        // ALIGN_1024_BYTES   = 1024
    };

    extern ALIGNMENT getAlignment(size_t alignment);

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
                                    size_t blockSize/*, size_t alignment*/);

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

        inline size_t getAlignment() const { return m_alignment; }

        inline size_t getBlockSize() const { return m_blockSize; }

        inline size_t getNumberOfBlocks() const { return m_numBlocks; }

        inline size_t getNumberOfFreeBlocks() const { return m_numFreeBlocks; }

        inline size_t getFreeMemory() const { return m_numFreeBlocks * m_blockSize; }

        /**
         * @return default container allocator
         */
        static ContainerAllocator& getDefaultContainerAllocator();

    private:
        void* allocate(size_t blocks);

        size_t calcNumBlocks(uintptr_t memoryPointer, size_t memorySizeBytes,
                            size_t blockSize, size_t alignment);

        inline void* blockIndexToPointer(size_t blockIndex) const {
            uintptr_t pointer = reinterpret_cast<uintptr_t>(m_memoryPointer) + 
                                m_initialPadding +
                                (blockIndex * m_blockSize);
            return reinterpret_cast<void*>(pointer);
        }

        inline size_t pointerToBlockIndex(void* pointer) const {
            uintptr_t ptr = reinterpret_cast<uintptr_t>(pointer);
            uintptr_t memStart = reinterpret_cast<uintptr_t>(m_memoryPointer) + 
                                 m_initialPadding;

            assert(ptr >= memStart);

            uintptr_t diff = ptr - memStart;
            
            return diff / m_blockSize;
        }

        inline size_t & nextIndex(size_t const & index) {
            return *reinterpret_cast<size_t*>(&(reinterpret_cast<unsigned char*>
                                               (m_paddedMemoryPointer)[index * m_blockSize]));
        }

        void* m_memoryPointer;
        void* m_paddedMemoryPointer;

        // Size of block.
        size_t m_blockSize;
        // Data alignment.
        // size_t _alignment;
        static constexpr size_t m_alignment = alignof(size_t);
        // Number of blocks.
        size_t m_numBlocks;
        // Padding at the start of the memory
        size_t m_initialPadding;
        // Number of free blocks
        size_t m_numFreeBlocks;
        // Index of the first free block
        // If this is greater than or equal to
        // m_numBlocks, there are no free blocks
        size_t m_firstFreeBlockIndex;
    };
}

#endif