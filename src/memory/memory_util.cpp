#include "memory_util.h"

size_t prt::memory_util::calcPadding(uintptr_t memoryPointer, size_t alignment) {
        assert(alignment >= 1);
        // assert(alignment <= 128);
        assert(alignment <= 256);
        assert((alignment & (alignment - 1)) == 0); // verify power of 2

        // Allocate unaligned block & convert address to uintptr_t.
        uintptr_t rawAddress = reinterpret_cast<uintptr_t>(memoryPointer);
        
        // Calculate the adjustment by masking off the lower bits
        // of the address, to determine how "misaligned" it is.
        size_t mask = (alignment - 1);
        size_t misalignment = (rawAddress & mask);
        size_t padding = misalignment == 0 ? 0 : alignment - misalignment;

        return padding;
    }