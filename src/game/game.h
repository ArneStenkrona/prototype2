#ifndef GAME_H
#define GAME_H

#include "src/system/assets/model_manager.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/scene/scene.h"
#include "src/game/system/physics/physics_system.h"

#include "src/graphics/camera/camera.h"
#include "src/graphics/vulkan/game_renderer.h"
#include "src/system/input/input.h"

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    Input m_input;
    GameRenderer m_gameRenderer;

    AssetManager m_assetManager;

    PhysicsSystem m_physicsSystem;
    Scene m_scene;

    uint32_t m_frameRate;
    uint32_t m_microsecondsPerFrame;

    uint64_t m_currentFrame;
    float m_time;

    void update(float deltaTime);

    void loadScene();
};

#endif