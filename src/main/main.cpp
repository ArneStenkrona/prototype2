#include <stdio.h>
#include <iostream>
#include "src/config/prototype2Config.h"
#include "src/memory/stack_allocator.h"
#include "src/graphics/vulkan/vulkan_application.h"

int main (int /*argc*/, char * /*argv*/ []) {
    std::cout << "Hello, world!" << std::endl;
    auto mem = ENTITY_MANAGER_STACK_SIZE_BYTES;
    std::cout << "Memory: " << mem << std::endl;

    vmain();

    return 0;
}