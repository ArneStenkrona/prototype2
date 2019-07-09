#include <stdio.h>
#include <iostream>
#include "src/config/prototype2Config.h"
#include "src/memory/stack_allocator.h"
#include <vulkan/vulkan.h>

int main (int /* argc */, char */* argv */[]){
    std::cout << "Hello, world!" << std::endl;
    auto mem = GLOBAL_MEMORY_ALLOCATION_MEGABYTES;
    std::cout << "Memory: " << mem << std::endl;

    StackAllocator stackAllocator(100000);
    int* x = static_cast<int*>
            (stackAllocator.allocAligned(10 * sizeof(int), sizeof(int)));
    
    for (int i = 0; i < 10; i++) {
        x[i] = i * i;
    }

    for (int i = 0; i < 10; i++) {
        std::cout << x[i] << std::endl;
    }

    return 0;
}