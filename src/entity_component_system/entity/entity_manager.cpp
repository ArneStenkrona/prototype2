#include "entity_manager.h"

#include <cassert>

EntityManager::EntityManager(void* memoryPointer, size_t  memorySizeBytes) 
    : _nextID(0), _stackAllocator(StackAllocator(memoryPointer, memorySizeBytes)) {
    // Number of different components
    // This is possible due to all component types are known at compile time
    static ComponentTypeID numDiffComponents = IComponentManager::totalNumTypes();

    // resize component manager container
    componentManagers.resize(numDiffComponents, nullptr);
}

EntityID EntityManager::createEntity() {
    EntityID id = _nextID;
    assert(id < _entities.size());
    _entities[id] = true;
    _nextID++;
    return id;
}

//void EntityManager::destroyEntity(EntityID entityID) {
//    // TODO make sure all components are removed as well
//    _entities[entityID] = false;
//}