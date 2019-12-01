#ifndef GAME_H
#define GAME_H

#include "src/entity_component_system/entity/entity_manager.h"
#include "src/entity_component_system/system/render_system.h"
#include "src/system/assets/model_manager.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/scene/scene.h"

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

    AssetManager _assetManager;

    Scene _scene;

    Camera _camera;

    uint32_t _frameRate;
    uint32_t _microsecondsPerFrame;

    uint64_t _currentFrame;

    void update(float deltaTime);
    void updateGraphics();
};

#endif