#include "entity_manager.h"

#include <cassert>

EntityManager::EntityManager(void* memoryPointer, size_t  memorySizeBytes) 
    : _nextID(0), _stackAllocator(StackAllocator(memoryPointer, memorySizeBytes)) {
        _entititesSize = UINT16_MAX;
        _entities = reinterpret_cast<bool*>(
            _stackAllocator.
            allocate(_entititesSize * sizeof(bool), sizeof(bool)));
}

EntityID EntityManager::createEntity() {
    EntityID id = _nextID;
    assert(id < _entititesSize);
    _entities[id] = true;
    _nextID++;
    return id;
}

//void EntityManager::destroyEntity(EntityID entityID) {
//    // TODO make sure all components are removed as well
//    _entities[entityID] = false;
//}