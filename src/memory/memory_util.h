#ifndef MEMORY_UTIL_H
#define MEMORY_UTIL_H

#include <assert.h>

namespace prt { namespace memory_util {
    /**
     * Given a memory address, calculates the
     * smallest postive value to be added
     * in order to obtain an aligned address
     * @param memoryPointer address
     * @param alignment alignment
     * 
     * @return padding
     */ 
    size_t calcPadding(uintptr_t memoryPointer, size_t alignment);

} }
#endif