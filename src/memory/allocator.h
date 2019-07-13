#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include <cstdint>

class Allocator {
public:
    virtual void* allocate(size_t size, size_t alignment) = 0;
    virtual void free(void* pointer) = 0;
    /**
     * Clears all memory, effectively resetting the allocator
     */
    virtual void clear() = 0;

protected:
    Allocator(void* memoryPointer, size_t  memorySizeBytes)
        : _memoryPointer(reinterpret_cast<uintptr_t>(memoryPointer)), _memorySizeBytes(memorySizeBytes) {}

    // Pointer to the start of the memory of the allocator.
    uintptr_t _memoryPointer;
    // Amount of memory available to the allocator in bytes.
    const size_t _memorySizeBytes;
};

#endif