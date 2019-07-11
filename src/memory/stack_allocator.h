#include "allocator.h"

#include <cstddef>

class StackAllocator : public Allocator {
public:

    typedef size_t Marker;

    /**
     *  Constructs a stack allocator with the given total size.
     *  @param stackSize_bytes size of stack in bytes
     */
    explicit StackAllocator(size_t stackSize_bytes);

    /** 
     * Allocates a new block of the given size from stack top
     * with respect to alignment.
     * @param size_bytes size of block in bytes
     * @param alignment aligmnet in bytes
     * 
     * @return pointer to allocated block
     */
    void* allocate(size_t size_bytes, size_t alignment) override;

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
     * Rolls the stack back to the previous marker.
     * @param marker
     */
    void freeToMarker(Marker marker);

    /**
     * Clears the entire stack (rolls the stack back to zero) .
     */
    void clear();

private:
    // Stack marker: represents the current top of the
    // stack. You can only roll back to a marker, not to
    // arbitrary locations within the stack.
    Marker _stackMarker;
    // Size of the stack
    const size_t _stackSize_bytes;

    // Pointer to the start of the stack
    void* _stackPointer;
    
    /**
     * Allocates a new block of the given size from stack top.
     * No alignment is taken into consideration
     * @param size_bytes size of block in bytes
     * 
     * @return  pointer to allocated block
     */
    void* allocUnaligned(size_t size_bytes);

    /**
     * Roll back the stack pointer to before the location
     * pointed at by pointer. Alignment is NOT considered
     * @param pointer
     */
    void freeUnaligned(void* pointer);
};