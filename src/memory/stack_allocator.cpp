#include "stack_allocator.h"

#include <stdlib.h>
#include <cassert>

StackAllocator::StackAllocator(size_t stackSize_bytes)
: _stackMarker(0), _stackSize_bytes(stackSize_bytes) {
    _stackPointer = malloc(stackSize_bytes);
}

void* StackAllocator::allocAligned(size_t size_bytes, size_t alignment) {
    assert(alignment >= 1);
    assert(alignment <= 128);
    assert((alignment & (alignment - 1)) == 0); // verify power of 2

    // Determine total amount of memory to allocate.
    size_t expandSize_bytes = size_bytes + alignment;

    // Assert that the stack can allocate the block
    assert(_stackMarker + expandSize_bytes <= _stackSize_bytes);

    // cast so we can perform arithmetic on stack pointer
    uintptr_t sPtr = reinterpret_cast<uintptr_t>(_stackPointer);

    // Allocate unaligned block & convert address to uintptr_t.
    uintptr_t rawAdress = reinterpret_cast<uintptr_t>(sPtr + _stackMarker);

    // Calculate the adjustment by masking off the lower bits
    // of the address, to determine how "misaligned" it is.
    size_t mask = (alignment - 1);
    uintptr_t misalignment = (rawAdress & mask);
    ptrdiff_t adjustment = alignment - misalignment;

    // Calculate the adjusted address, return as pointer.
    uintptr_t alignedAddress = rawAdress + adjustment;

    // Store the adjustment in the byte immediately
    // preceding the adjusted address.
    assert(adjustment < 256);
    uint8_t* pAlignedMem = reinterpret_cast<uint8_t*>(alignedAddress);
    pAlignedMem[-1] = static_cast<uint8_t>(adjustment);

    // Increment the stack marker
    _stackMarker += size_bytes + static_cast<size_t>(adjustment);

    return static_cast<void*> (pAlignedMem);
}

void* StackAllocator::allocUnaligned(size_t size_bytes) {
     // Assert that the stack can allocate the block
    assert(_stackMarker + size_bytes <= _stackSize_bytes);

    // cast so we can perform arithmetic on stack pointer
    uintptr_t sPtr = reinterpret_cast<uintptr_t>(_stackPointer);

    // Find the address
    void* address = reinterpret_cast<void*>(sPtr + _stackMarker);
    // Increment the stack marker
    _stackMarker += size_bytes;

    return address;
}

void StackAllocator::freeAligned(void* pMem) {
        const uint8_t* pAlignedMem = reinterpret_cast<const uint8_t*>(pMem);

        uintptr_t alignedAddress = reinterpret_cast<uintptr_t>(pMem);
        ptrdiff_t adjustment = static_cast<ptrdiff_t>(pAlignedMem[-1]);

        uintptr_t rawAdress = alignedAddress - adjustment;
        void* pRawMem = reinterpret_cast<void*>(rawAdress);

        freeUnaligned(pRawMem);
    }

void StackAllocator::freeUnaligned(void* pointer) {
    uintptr_t ptr = reinterpret_cast<uintptr_t>(pointer);
    uintptr_t sPtr = reinterpret_cast<uintptr_t>(_stackPointer);

    ptrdiff_t diff = ptr - sPtr;
    // Make sure that the pointer is within the stack
    assert(ptr > sPtr);
    assert(diff > 0);
    assert(static_cast<size_t>(diff) < _stackSize_bytes);

    // The updated stack marker should point to the 
    // Marker that the pointer mapps to
    _stackMarker = static_cast<size_t>(diff);
}

void StackAllocator::freeToMarker(Marker marker) {
    _stackMarker = marker;
}

void StackAllocator::clear() {
    _stackMarker = 0;
}