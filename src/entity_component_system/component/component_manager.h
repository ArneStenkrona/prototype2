#ifndef COMPONENT_MANAGER_H
#define COMPONENT_MANAGER_H

#include "src/memory/stack_allocator.h"

#include <cstddef> 
#include <typeindex>
#include <typeinfo>
#include <iostream>

typedef uint16_t EntityID;
typedef uint16_t ComponentTypeID;

class AbstractComponentManager {
public:
    //virtual ~AbstractComponentManager() = 0;
protected:
    static ComponentTypeID getID() { return _nextTypeID++; }
    static ComponentTypeID _nextTypeID;
};

template<class Component>
class ComponentManager : public AbstractComponentManager {
public:    
    // ID indicating type of component manager
    static const ComponentTypeID STATIC_COMPONENT_TYPE_ID;

    ComponentManager(void* memoryPointer, size_t  memorySizeBytes)
        : _stack(StackAllocator(memoryPointer, memorySizeBytes)) {
        // Did we get enough memory?
        assert(memorySizeBytes >= UINT16_MAX * sizeof(Component) + 
                                  2 * UINT16_MAX * sizeof(Component) );

        _entityIDToComponent = 
            reinterpret_cast<uint16_t*>(_stack.
            allocate(UINT16_MAX * sizeof(uint16_t),
                     sizeof(uint16_t)));
                     
        _componentToEntityID = 
            reinterpret_cast<EntityID*>(_stack.
            allocate(UINT16_MAX * sizeof(EntityID),
                     sizeof(EntityID)));

        for (uint16_t i = 0; i < UINT16_MAX; i++) {
            _entityIDToComponent[i] = 
            _componentToEntityID[i] =
                                      UNDEFINED_MAP;
        }

        _components = 
            reinterpret_cast<Component*>(_stack.
            allocate(UINT16_MAX * sizeof(Component),
                     sizeof(Component)));
    }

    //virtual ~ComponentManager(){}

private:
    // Denotes an undefined mapping
    // This effectively limits the amount of components/entities
    // to (2^16) - 1.
    static constexpr uint16_t UNDEFINED_MAP = static_cast<uint16_t>(-1);

    // Array mappings
    // Systems should internally cache these to avoid
    // d-cache misses due to their sparsity
    uint16_t* _entityIDToComponent; // Initialize to UNDEFINED_MAP
    EntityID* _componentToEntityID; // Initialize to UNDEFINED_MAP
    uint16_t _numComponents;
    Component* _components;

    StackAllocator _stack;

    void addComponent(EntityID entityID) {
        assert(_entityIDToComponent[entityID] == UNDEFINED_MAP);
        _entityIDToComponent[entityID] = _numComponents;
        _componentToEntityID[_numComponents] = entityID;
        _components[_numComponents] = Component();
        _numComponents++;
    }

    inline Component getComponent(EntityID entityID) {
        uint16_t index = _entityIDToComponent[entityID];
        assert(index != UNDEFINED_MAP);
        return _components[index];
    }

    void removeComponent(EntityID entityID) {
        uint16_t index = _entityIDToComponent[entityID];
        assert(index != UNDEFINED_MAP);
        _componentToEntityID[index] = UNDEFINED_MAP;
        _entityIDToComponent[entityID] = UNDEFINED_MAP;
    }

    friend class EntityManager;
};

template<class T>
const ComponentTypeID ComponentManager<T>::STATIC_COMPONENT_TYPE_ID = 
                                    AbstractComponentManager::getID();


#endif