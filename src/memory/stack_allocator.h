#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "allocator.h"

#include <cstddef>

class StackAllocator : public Allocator {
public:

    typedef size_t Marker;

     /**
     * Constructs a stack allocator with the given total size.
     * The caller is responsible for ensuring that memoryPointer
     * points to a valid, free memory block with size of
     * memorySizeBytes
     * 
     * @param memoryPointer pointer to the first memory address
     *          of the allocator
     * @param memorySizeBytes size of memory in bytes
     */
    explicit StackAllocator(void* memoryPointer, size_t  memorySizeBytes);

    /** 
     * Allocates a new block of the given size from stack top
     * with respect to alignment.
     * @param size_bytes size of block in bytes
     * @param alignment aligmnet in bytes, must be power of two
     * 
     * @return pointer to allocated block
     */
    void* allocate(size_t sizeBytes, size_t alignment) override;

    /**
     * Roll back the stack pointer to before the location
     * pointed at by pointer. Alignment is considered
     * @param pointer
     */
    void free(void* pointer) override;

    /**
     * Returns a marker to the current stack top. 
     */
    inline Marker getMarker() const { return _stackMarker; }

    /**
     * Clears the entire stack (rolls the stack back to zero) .
     */
    void clear();

private:
    // Stack marker: represents the current top of the
    // stack. You can only roll back to a marker, not to
    // arbitrary locations within the stack.
    Marker _stackMarker;
    
    /**
     * Allocates a new block of the given size from stack top.
     * No alignment is taken into consideration
     * @param size_bytes size of block in bytes
     * 
     * @return  pointer to allocated block
     */
    void* allocUnaligned(size_t sizeBytes);

    /**
     * Roll back the stack pointer to before the location
     * pointed at by pointer. Alignment is NOT considered
     * @param pointer
     */
    void freeUnaligned(void* pointer);
};

#endif