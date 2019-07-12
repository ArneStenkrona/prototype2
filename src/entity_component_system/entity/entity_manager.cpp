#include "entity_manager.h"

EntityManager::EntityManager() 
    : _nextID(0), _entities{false} {

        

}

EntityID EntityManager::createEntity() {
    EntityID id = _nextID;
    assert(id < ENTITY_BUFFER_SIZE);
    _entities[id] = true;
    _nextID++;
    return id;
}

void EntityManager::destroyEntity(EntityID entityID) {
    // TODO make sure all components are removed as well
    _entities[entityID] = false;
}