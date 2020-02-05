#include "game.h"

#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <chrono>
#include <thread>

#include <iostream>

Game::Game()
: _input(),
  _vulkanApp(_input),
  _assetManager(RESOURCE_PATH),
  _camera(_input),
  _scene(_assetManager, _physicsSystem, _input, _camera),
  _frameRate(FRAME_RATE),
  _microsecondsPerFrame(1000000 / _frameRate),
  _currentFrame(0),
  _time(0.0f) {

    _input.init(_vulkanApp.getWindow());
    _scene.load(_vulkanApp);
}

Game::~Game() {
    _vulkanApp.cleanup();
}

void Game::run() {
    using clock = std::chrono::high_resolution_clock;
    //static auto startTime = std::chrono::high_resolution_clock::now();
    auto lastTime = clock::now();
    clock::time_point deadLine = clock::now();

    uint32_t framesMeasured = 0;
    clock::time_point nextSecond = lastTime + std::chrono::seconds(1);
        
    while (_vulkanApp.isWindowOpen()) {
        deadLine = deadLine + std::chrono::microseconds(_microsecondsPerFrame);
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

        _currentFrame++;
    }
}

void Game::update(float deltaTime) {
    _time += deltaTime;
    _input.update();
    _camera.update(deltaTime);
    _scene.update(deltaTime);
    updateGraphics(deltaTime);
}

void Game::updateGraphics(float /*deltaTime*/) {
    prt::vector<glm::mat4> modelMatrices; 
    _scene.getTransformMatrices(modelMatrices);

    glm::mat4 viewMatrix = _camera.getViewMatrix();
    int w,h = 0;
    _vulkanApp.getWindowSize(w, h);
    glm::mat4 projectionMatrix = _camera.getProjectionMatrix(float(w), float(h), 0.1f, 200.0f);
    glm::mat4 skyProjectionMatrix = _camera.getProjectionMatrix(float(w), float(h), 0.1f, 1000.0f);
    glm::vec3 viewPosition = _camera.getPosition();
    _vulkanApp.update(modelMatrices, viewMatrix, projectionMatrix, viewPosition, 
                      skyProjectionMatrix, _time);
}