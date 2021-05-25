#ifndef GAME_H
#define GAME_H

#include "src/system/assets/model_manager.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/scene/scene.h"
#include "src/game/editor/editor.h"
#include "src/game/system/physics/physics_system.h"
#include "src/graphics/camera/camera.h"
#include "src/graphics/vulkan/game_renderer.h"
#include "src/graphics/vulkan/game_renderer.h"


class Game {
public:
    Game();
    ~Game();

    void run();

private:
    enum Mode {
        GAME,
        EDITOR
    };

    Mode m_mode = Mode::GAME;
    RenderGroupMask m_renderMask = RENDER_GROUP_FLAG_ALL;

    Input m_input;
    GameRenderer m_gameRenderer;

    AssetManager m_assetManager;

    PhysicsSystem m_physicsSystem;

    Scene m_scene;
    Editor m_editor;

    uint32_t m_frameRate;
    uint32_t m_microsecondsPerFrame;

    uint64_t m_currentFrame;
    float m_time;

    static constexpr int DEFAULT_WIDTH = 800;
    static constexpr int DEFAULT_HEIGHT = 600;

    void update(float deltaTime);
    void updateMode();

    void loadScene();
};

#endif