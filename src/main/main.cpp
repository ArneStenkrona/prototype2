#include <stdio.h>
#include <iostream>
#include "src/config/prototype2Config.h"
#include "src/memory/stack_allocator.h"
#include "src/game/game.h"

int main (int /*argc*/, char * /*argv*/ []) {
    std::cout << "Hello, world!" << std::endl;
    auto mem = ENTITY_MANAGER_STACK_SIZE_BYTES;
    std::cout << "Memory: " << mem << std::endl;
    
    Game game;

    try {
        game.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}