#ifndef COMPONENT_MANAGER_H
#define COMPONENT_MANAGER_H

#include "src/container/array.h"
#include "src/container/vector.h"

#include "src/config/prototype2Config.h"
#include <cstddef> 
#include <typeindex>
#include <typeinfo>

typedef uint16_t EntityID;
typedef uint16_t ComponentTypeID;

class IComponentManager {
public:
    //virtual ~IComponentManager() = 0;
    static ComponentTypeID totalNumTypes() { return _nextTypeID; }
protected:
    static ComponentTypeID getID() { return _nextTypeID++; }
    static ComponentTypeID _nextTypeID;
};

template<class ComponentType>
class ComponentManager : public IComponentManager {
public:    
    // ID indicating type of component manager
    static const ComponentTypeID STATIC_COMPONENT_TYPE_ID;

    ComponentManager() {
        std::fill(_entityIDToComponent.begin(), _entityIDToComponent.end(), UNDEFINED_MAP);
        std::fill(_componentToEntityID.begin(), _componentToEntityID.end(), UNDEFINED_MAP);
    }

    //virtual ~ComponentManager(){}

private:
    // Denotes an undefined mapping
    // This effectively limits the amount of components/entities
    // to (2^16) - 1.
    static constexpr uint16_t UNDEFINED_MAP = static_cast<uint16_t>(-1);

    // Array mappings
    prt::array<uint16_t, TOTAL_ENTITIES> _entityIDToComponent;
    prt::array<EntityID, TOTAL_ENTITIES> _componentToEntityID;
    prt::vector<ComponentType> _components;

    void addComponent(EntityID entityID) {
        assert(_entityIDToComponent[entityID] == UNDEFINED_MAP);
        _entityIDToComponent[entityID] = _components.size();
        _componentToEntityID[_components.size()] = entityID;
        _components.push_back(ComponentType());
    }

    inline ComponentType& getComponent(EntityID entityID) const {
        uint16_t index = _entityIDToComponent[entityID];
        assert(index != UNDEFINED_MAP);
        return &_components[index];
    }

    void removeComponent(EntityID entityID) {
        uint16_t index = _entityIDToComponent[entityID];
        assert(index != UNDEFINED_MAP);
        _componentToEntityID[index] = UNDEFINED_MAP;
        _entityIDToComponent[entityID] = UNDEFINED_MAP;
    }

    bool hasComponent(EntityID entityID) {
        return _entityIDToComponent[entityID] != UNDEFINED_MAP;
    } 

    friend class EntityManager;
    friend class Game;
};

template<class ComponentType>
const ComponentTypeID ComponentManager<ComponentType>::STATIC_COMPONENT_TYPE_ID = 
                                    IComponentManager::getID();


#endif