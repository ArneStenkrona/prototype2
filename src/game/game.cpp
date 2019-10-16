#include "game.h"

#include "src/container/vector.h"
#include "src/entity_component_system/component/component.h"
#include "src/config/prototype2Config.h"

#include <chrono>

Game::Game()
: _vulkanApp(),
  _input(_vulkanApp.getWindow()),
  _modelManager((RESOURCE_PATH + std::string("models/")).c_str()),
  _staticEntityManager(),
  _camera(_input) {
    prt::vector<std::string> paths;
    _modelManager.getPaths(paths);
    _vulkanApp.loadModels(paths);
    prt::vector<uint32_t> modelIDs;
    _staticEntityManager.getModelIDs(modelIDs);
    _vulkanApp.bindStaticEntities(modelIDs);
}

Game::~Game() {
    _vulkanApp.cleanup();
}

void Game::run() {
        //static auto startTime = std::chrono::high_resolution_clock::now();
        auto lastTime = std::chrono::high_resolution_clock::now();
    while (_vulkanApp.isWindowOpen()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;
        _input.update();
        _camera.update(deltaTime);
        prt::vector<glm::mat4> modelMatrices; 
        _staticEntityManager.getTransformMatrixes(modelMatrices);

        glm::mat4 viewMatrix = _camera.getViewMatrix();
        glm::mat4 projectionMatrix = _camera.getProjectionMatrix();
        _vulkanApp.update(modelMatrices, viewMatrix, projectionMatrix);
    }
}