#include "game.h"

#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <chrono>
#include <thread>

#include <iostream>

Game::Game()
: m_input(),
  m_gameRenderer(800, 600),
  m_assetManager(RESOURCE_PATH),
  m_camera(m_input),
  m_physicsSystem(m_assetManager.getModelManager()),
  m_scene(m_assetManager, m_physicsSystem, m_input, m_camera),
  m_frameRate(FRAME_RATE),
  m_microsecondsPerFrame(1000000 / m_frameRate),
  m_currentFrame(0),
  m_time(0.0f) {
    m_input.init(m_gameRenderer.getWindow());
    loadScene();
}

void Game::loadScene() {
    m_scene.bindToRenderer(m_gameRenderer);
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

        std::this_thread::sleep_until(deadLine);

        framesMeasured++;
        if (nextSecond <= clock::now()) {
            nextSecond += std::chrono::seconds(1);
            std::cout << "Frame rate: " << framesMeasured << "FPS" << std::endl;
            framesMeasured = 0;
        }

        m_currentFrame++;
    }
}

void Game::update(float deltaTime) {
    m_time += deltaTime;
    m_input.update();
    m_camera.update(deltaTime);
    m_scene.update(deltaTime);
    updateGraphics(deltaTime);
}

void Game::updateGraphics(float /*deltaTime*/) {
    prt::vector<glm::mat4> modelMatrices; 
    m_scene.getTransformMatrices(modelMatrices, false);
    prt::vector<glm::mat4> animatedModelMatrices; 
    m_scene.getTransformMatrices(animatedModelMatrices, true);

    prt::vector<glm::mat4> bones; 
    m_scene.sampleAnimation(bones);

    auto const & sun = m_scene.getSun();

    m_gameRenderer.update(modelMatrices, 
                          animatedModelMatrices,
                          bones,
                          m_camera, 
                          sun,
                          m_time);
}