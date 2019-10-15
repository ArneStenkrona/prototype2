#ifndef GAME_H
#define GAME_H

#include "src/entity_component_system/entity/entity_manager.h"
#include "src/entity_component_system/system/render_system.h"
#include "src/system/assets/model_manager.h"

#include "src/graphics/camera/camera.h"
#include "src/graphics/vulkan/vulkan_application.h"
#include "src/system/input/input.h"

class Game {
public:
    Game();
    ~Game();

    void run();
private:
    VulkanApplication _vulkanApp;
    Input _input;
    //EntityManager _entityManager;
    //RenderSystem _renderSystem;
    ModelManager _modelManager;

    Camera _camera;
};

#endif