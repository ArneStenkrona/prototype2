#include "game.h"

#include "src/container/vector.h"
#include "src/entity_component_system/component/component.h"
#include "src/config/prototype2Config.h"

Game::Game()
: _vulkanApp(),
  _modelManager((RESOURCE_PATH + std::string("models/")).c_str()),
  _camera(_vulkanApp.getWindow()) {
    prt::vector<std::string> paths;
    _modelManager.getPaths(paths);
    _vulkanApp.loadModels(paths);
}

Game::~Game() {
    _vulkanApp.cleanup();
}

void Game::run() {
    while (_vulkanApp.isWindowOpen()) {
        _camera.update(0.1f);
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = _camera.getViewMatrix();
        glm::mat4 projectionMatrix = _camera.getProjectionMatrix();
        _vulkanApp.update(modelMatrix, viewMatrix, projectionMatrix);
    }
}

void Game::update() {
    //prt::vector<Transform> transforms;
    //prt::vector<Model> models;
    //ComponentManager<Transform>& transformManager =
    //    _entityManager.getComponentManager<Transform>();
    //ComponentManager<Model>& modelManager =
    //    _entityManager.getComponentManager<Model>();
    //_renderSystem.update(modelManager._components, modelManager._componentToEntityID,
    //                    transformManager._components, transformManager._entityIDToComponent);
}