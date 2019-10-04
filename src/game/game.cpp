#include "game.h"

#include "src/container/vector.h"

#include "src/entity_component_system/component/component.h"

Game::Game(void* memoryPointer, size_t  memorySizeBytes)
: entityManager(memoryPointer, memorySizeBytes),
  renderSystem() {

}

void Game::run() {
    while (true /*<- use condition instead*/) {
        update();
    }
}

void Game::update() {
    prt::vector<Transform> transforms;
    prt::vector<Model> models;
    ComponentManager<Transform>& transformManager =
        entityManager.getComponentManager<Transform>();
    ComponentManager<Model>& modelManager =
        entityManager.getComponentManager<Model>();
    renderSystem.update(modelManager._components, modelManager._componentToEntityID,
                        transformManager._components, transformManager._entityIDToComponent);
}