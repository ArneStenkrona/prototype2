#include "container_allocator.h"
#include "src/config/prototype2Config.h"

#include "memory_util.h"

#include <string.h>

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
                        : m_memoryPointer(memoryPointer),
                          m_paddedMemoryPointer(nullptr),
                          m_blockSize(blockSize),
                          /*_alignment(alignment),*/
                          m_numBlocks(calcNumBlocks(reinterpret_cast<uintptr_t>(memoryPointer),
                                                   memorySizeBytes, blockSize, m_alignment)),
                          m_initialPadding(prt::memory_util::calcPadding(reinterpret_cast<uintptr_t>(memoryPointer),
                                                                        m_alignment)),
                          m_numFreeBlocks(m_numBlocks),
                          m_firstFreeBlockIndex(0) {
    assert(m_alignment > 0);
    assert(m_blockSize >= sizeof(size_t));
    assert(m_alignment <= m_blockSize);
    // assert(alignment <= 128);
    assert(m_alignment <= 256);
    assert(m_blockSize % m_alignment == 0);
    assert((m_alignment & (m_alignment - 1)) == 0); // verify power of 2

    uintptr_t memPtr = reinterpret_cast<uintptr_t>(memoryPointer) + m_initialPadding;
    m_paddedMemoryPointer = reinterpret_cast<void*>(memPtr);

    for (size_t i = 0; i < m_numBlocks; ++i) {
       nextIndex(i) = i + 1;
    } 
}

void* prt::ContainerAllocator::allocate(size_t sizeBytes, size_t alignment) {
    assert(alignment + sizeof(size_t) <= m_blockSize);
    assert(alignment >= 1);
    // assert(alignment <= 128);
    assert(alignment <= 256);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2
    assert((sizeBytes <= m_numFreeBlocks * m_blockSize));

    // allocated enough to store number of blocks + sizeBytes + padding
    size_t blocks = alignment <= m_alignment ? 
                    (sizeof(size_t) + sizeBytes + m_blockSize - 1) / m_blockSize :
                    (sizeof(size_t) + sizeBytes + alignment + m_blockSize - 1) / m_blockSize;
    
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
    size_t freed = *reinterpret_cast<size_t*>(mem);
    m_numFreeBlocks += freed;
    
    size_t *pCurr = &m_firstFreeBlockIndex;
    while (*pCurr < blockIndex) {
        pCurr = &nextIndex(*pCurr);
    }
    size_t next = *pCurr;
    *pCurr = blockIndex;
    while (freed > 1) {
        nextIndex(blockIndex) = blockIndex + 1;
        ++blockIndex;
        --freed;
    }
    nextIndex(blockIndex) = next;
}

void prt::ContainerAllocator::clear() {
    m_numFreeBlocks = m_numBlocks;
    m_firstFreeBlockIndex = 0;
    for (size_t i = 0; i < m_numBlocks; ++i) {
        nextIndex(i) = i + 1;
    } 
}

void* prt::ContainerAllocator::allocate(size_t blocks) {
    assert(blocks > 0);
    assert(blocks <= m_numBlocks);
    assert(m_numFreeBlocks >= blocks);
    assert(m_firstFreeBlockIndex < m_numBlocks);
    // Find suitable range of blocks. O(n)
    size_t curr = m_firstFreeBlockIndex;
    size_t index = m_firstFreeBlockIndex;
    size_t *pToIndex = &m_firstFreeBlockIndex;
    size_t blocksInARow = 0;
    while (curr < m_numBlocks) {
        ++blocksInARow;
        size_t *pNext = &nextIndex(curr);
        if (blocksInARow == blocks) {
            m_numFreeBlocks -= blocks;
            *pToIndex = *pNext;

            return blockIndexToPointer(index);
        }

        if (*pNext - curr > 1) {
            blocksInARow = 0;
            index = *pNext;
            pToIndex = pNext;
        }
        curr = *pNext;
    }

    assert(false);
    return nullptr;
}