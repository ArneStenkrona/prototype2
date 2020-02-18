#ifndef GAME_H
#define GAME_H

#include "src/entity_component_system/entity/entity_manager.h"
#include "src/system/assets/model_manager.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/scene/scene.h"
#include "src/game/system/physics_system.h"

#include "src/graphics/camera/camera.h"
#include "src/graphics/vulkan/game_renderer.h"
#include "src/system/input/input.h"

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    Input _input;
    GameRenderer _gameRenderer;

    AssetManager _assetManager;

    Camera _camera;
    PhysicsSystem _physicsSystem;
    Scene _scene;

    uint32_t _frameRate;
    uint32_t _microsecondsPerFrame;

    uint64_t _currentFrame;
    float _time;

    void update(float deltaTime);
    void updateGraphics(float deltaTime);

    void loadScene();
};

#endif