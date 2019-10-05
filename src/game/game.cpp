#include "game.h"

#include "src/container/vector.h"
#include "src/entity_component_system/component/component.h"
#include "src/config/prototype2Config.h"

Game::Game()
: _modelManager((RESOURCE_PATH + std::string("models/")).c_str()),
  _vulkanApp() {
    
}

Game::~Game() {
    _vulkanApp.cleanup();
}

void Game::run() {
    while (_vulkanApp.isWindowOpen()) {
        _vulkanApp.mainLoop();
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