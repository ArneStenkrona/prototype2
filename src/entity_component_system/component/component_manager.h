#ifndef COMPONENT_MANAGER_H
#define COMPONENT_MANAGER_H

#include "src/memory/stack_allocator.h"

#include <cstddef> 

typedef size_t EntityID;

template<class Component>
class ComponentManager {
public:
    ComponentManager(void* memoryPointer, size_t  memorySizeBytes)
        : _stack(StackAllocator(memoryPointer, memorySizeBytes)) {
        _entityIDmap = 
            reinterpret_cast<size_t*>(_stack.
            allocate(NUMBER_OF_SUPPORTED_ENTITIES * sizeof(size_t),
                     sizeof(size_t)));
        _components = 
            reinterpret_cast<size_t*>(_stack.
            allocate(NUMBER_OF_SUPPORTED_ENTITIES * sizeof(Component),
                     sizeof(Component)));
    }

private:
    constexpr size_t NUMBER_OF_SUPPORTED_ENTITIES;

    constexpr size_t UNDEFINED_MAP = static_cast<size_t>(-1);
    // Initialize to INVALID ID
    size_t* _entityIDmap;
    //EntityID* _entityIDs;
    size_t _numComponents;
    Component* _components;

    StackAllocator _stack;

    void addComponent(EntityID entityID) {
        assert(_entityIDmap[entityID] == UNDEFINED MAP)
        _entityIDmap[entityID] = _numComponents;
        _components[_numComponents] = Component();
        _numComponents++;
    }

    inline Component getComponent(EntityID entityID) {
        size_t index = _mapping[entityID];
        assert(index != UNDEFINED_MAP);
        return _components[index];
    }

    void removeComponent(EntityID entityID) {
        size_t index = _entityIDmap[entityID];
        assert(index != UNDEFINED MAP)
        _entityIDmap[index] = UNDEFINED_MAP;
    }

    friend class EntityManager;
};

#endif