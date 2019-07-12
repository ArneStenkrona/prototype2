#include "allocator.h"

class PoolAllocator : public Allocator {
public:
    /**
     * Constructs a pool allocator with the given total size.
     * The caller is responsible for ensuring that memoryPointer
     * points to a valid, free memory block with size of
     * memorySizeBytes
     * 
     * @param memoryPointer pointer to the first memory address
     *          of the allocator
     * @param memorySizeBytes size of memory in bytes
     */
    explicit PoolAllocator(void* memoryPointer, size_t memorySizeBytes);

    void* allocate(size_t size, size_t alignment) override;
    void free(void* pointer) override;
private:
    // size of block.
    const size_t _blockSize;
};