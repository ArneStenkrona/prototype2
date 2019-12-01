#include "game.h"

#include "src/container/vector.h"
#include "src/entity_component_system/component/component.h"
#include "src/config/prototype2Config.h"

#include <chrono>
#include <thread>

#include <iostream>

Game::Game()
: _vulkanApp(),
  _input(_vulkanApp.getWindow()),
  _assetManager(RESOURCE_PATH),
  //_modelManager((RESOURCE_PATH + std::string("models/")).c_str()),
  _scene(_assetManager),
  _camera(_input),
  _frameRate(FRAME_RATE),
  _microsecondsPerFrame(1000000 / _frameRate),
  _currentFrame(0) {
    prt::vector<Model> models;
    _assetManager.loadModels(models);
    _vulkanApp.loadModels(models);
    prt::vector<uint32_t> modelIDs;
    _scene.getModelIDs(modelIDs);
    _vulkanApp.bindStaticEntities(modelIDs);
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
    _input.update();
    _camera.update(deltaTime);
    updateGraphics();
}

void Game::updateGraphics() {
    prt::vector<glm::mat4> modelMatrices; 
    _scene.getTransformMatrixes(modelMatrices);

    glm::mat4 viewMatrix = _camera.getViewMatrix();
    glm::mat4 projectionMatrix = _camera.getProjectionMatrix();
    glm::vec3 viewPosition = _camera.getPosition();
    _vulkanApp.update(modelMatrices, viewMatrix, projectionMatrix, viewPosition);
}