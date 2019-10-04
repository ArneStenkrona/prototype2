#ifndef GAME_H
#define GAME_H

#include "src/entity_component_system/entity/entity_manager.h"
#include "src/entity_component_system/system/render_system.h"

class Game {
public:
    Game(void* memoryPointer, size_t  memorySizeBytes);

    void run();
private:
    void update();

    EntityManager entityManager;
    RenderSystem renderSystem;
};

#endif