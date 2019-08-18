#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H
//#include "entity.h"
#include "src/entity_component_system/component/component_manager.h"
#include "src/entity_component_system/component/component.h"

#include "src/memory/stack_allocator.h"
//#include "src/memory/pool_allocator.h"

#include "src/container/array.h"
#include "src/container/vector.h"

#include <cstddef> 
#include <cassert>
#include <new>

// Engine as it stands support a maximum of 2^16 - 1 entities
typedef uint16_t EntityID;

class EntityManager {
public:
    /**
     * @param memoryPointer pointer to the entity stack
     * @param memorySizeBytes size of the entity stack in bytes
     */
    EntityManager(void* memoryPointer, size_t  memorySizeBytes);

    EntityID createEntity();

    template<class ComponentType>
    void addComponent(EntityID entityID) {
        // ComponentType must be of Component Type
        assert(_entities[entityID]);
        ComponentManager<ComponentType>& componentManager = 
                        getComponentManager<ComponentType>();
        
        componentManager.addComponent(entityID);
    }

    template<class ComponentType>
    void removeComponent(EntityID entityID) {
        // ComponentType must be of Component Type
        ComponentManager<ComponentType>& componentManager = 
                        getComponentManager<ComponentType>();
    
        componentManager.removeComponent(entityID);
    }

    /**
     * Destroy an entity.
     * Note: as of yet this does not free
     * the entity slot
     * @param entityID
     */
    //void destroyEntity(EntityID entityID);
private:
   // Next ID to be issued to an entity.
    EntityID _nextID;
    
    // Stack allocator for the entity manager.
    StackAllocator _stackAllocator;

    //size_t _entitiesSize;
    //bool* _entities;
    prt::array<bool, UINT16_MAX> _entities;
    

    //static constexpr size_t NUMBER_OF_SUPPORTED_COMPONENT_TYPES = 50;
    //IComponentManager* componentManagers[NUMBER_OF_SUPPORTED_COMPONENT_TYPES];
    prt::vector<IComponentManager*> componentManagers;

    template<class ComponentType>
    ComponentManager<ComponentType>& getComponentManager() {
        static_assert(std::is_base_of<Component, ComponentType>::value, "ComponentType must be of base Component");
        ComponentTypeID cID = 
                    ComponentManager<ComponentType>::STATIC_COMPONENT_TYPE_ID;
                    
        // Check if manager already exists.
        if (componentManagers[cID] == nullptr) {
            // If not, create one.
            componentManagers[cID] =
                reinterpret_cast<ComponentManager<ComponentType>*>(_stackAllocator.
                                allocate(sizeof(ComponentManager<ComponentType>), 1));

            new(componentManagers[cID]) ComponentManager<ComponentType>();
            
        }
        return *(static_cast<ComponentManager<ComponentType>*>(componentManagers[cID]));      
    }
};

#endif