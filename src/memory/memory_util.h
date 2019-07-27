#ifndef MEMORY_UTIL_H
#define MEMORY_UTIL_H

#include <assert.h>

namespace prt { namespace memory_util {
    /**
     * Given a memory address, calculates the
     * smallest address greater than the pointer
     * with requested alignment.
     * @param memoryPointer address
     * @param alignment alignment
     * 
     * @return aligned address 
     */ 
    size_t calcPadding(uintptr_t memoryPointer, size_t alignment);

} }
#endif