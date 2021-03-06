#include "game.h"

#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <chrono>
#include <thread>

#include <iostream>

Game::Game()
: m_input(),
  m_gameRenderer(DEFAULT_WIDTH, DEFAULT_HEIGHT),
  m_assetManager(RESOURCE_PATH),
  m_physicsSystem(),
  m_scene(m_gameRenderer, m_assetManager, m_physicsSystem, m_input),
  m_editor(m_scene, m_input, m_gameRenderer.getWindow(), DEFAULT_WIDTH, DEFAULT_HEIGHT),
  m_frameRate(FRAME_RATE),
  m_microsecondsPerFrame(1000000 / m_frameRate),
  m_currentFrame(0),
  m_time(0.0f) {
    m_input.init(m_gameRenderer.getWindow());
    loadScene();
}

void Game::loadScene() {
    m_scene.bindToRenderer();
}

Game::~Game() {
}

void Game::run() {
    using clock = std::chrono::high_resolution_clock;
    //static auto startTime = std::chrono::high_resolution_clock::now();
    auto lastTime = clock::now();
    clock::time_point deadLine = clock::now();

    uint32_t framesMeasured = 0;
    clock::time_point nextSecond = lastTime + std::chrono::seconds(1);
        
    while (m_gameRenderer.isWindowOpen()) {
        deadLine = deadLine + std::chrono::microseconds(m_microsecondsPerFrame);
        auto currentTime = clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;
        update(deltaTime);

        m_gameRenderer.render(deltaTime, m_renderMask);   


        std::this_thread::sleep_until(deadLine);

        framesMeasured++;
        if (nextSecond <= clock::now()) {
            nextSecond += std::chrono::seconds(1);
            std::cout << "Frame rate: " << framesMeasured << "FPS" << std::endl;
            framesMeasured = 0;
        }

        m_currentFrame++;

        glfwPollEvents();
    }
}

void Game::update(float deltaTime) {
    m_time += deltaTime;
    m_input.update(m_mode == Mode::GAME);
    updateMode();
    switch (m_mode) {
        case Mode::GAME:
            m_renderMask = GameRenderer::GAME_RENDER_MASK;
            m_scene.update(deltaTime);
            break;
        case Mode::EDITOR:
            int w,h;
            m_gameRenderer.getWindowSize(w,h);
            m_renderMask = GameRenderer::EDITOR_RENDER_MASK;
            m_editor.update(deltaTime,w,h);
            break;
    }
}

void Game::updateMode() {
    if (m_input.getKeyDown(INPUT_KEY::KEY_TAB)) {
        m_mode = m_mode == Mode::GAME ? Mode::EDITOR : Mode::GAME; 
    }
}
