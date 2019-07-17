#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H
//#include "entity.h"
#include "src/entity_component_system/component/component_manager.h"
#include "src/entity_component_system/component/component.h"

#include "src/memory/stack_allocator.h"
#include "src/memory/pool_allocator.h"

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

    template<class T>
    void addComponent(EntityID entityID) {
        // T must be of Component Type
        assert(_entities[entityID]);
        ComponentManager<T>& componentManager = getComponentManager<T>();
        
        componentManager.addComponent(entityID);
    }

    template<class T>
    void removeComponent(EntityID entityID) {
        // T must be of Component Type
        ComponentManager<T>& componentManager = getComponentManager<T>();
    
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

    size_t _entititesSize;
    bool* _entities;

    static constexpr size_t NUMBER_OF_SUPPORTED_COMPONENT_TYPES = 50;
    AbstractComponentManager* componentManagers[NUMBER_OF_SUPPORTED_COMPONENT_TYPES];

    template<class T>
    ComponentManager<T>& getComponentManager() {
        static_assert(std::is_base_of<Component, T>::value, "T must be of base Component");
        ComponentTypeID cID = 
                    ComponentManager<T>::STATIC_COMPONENT_TYPE_ID;

        // Check if manager already exists.
        if (componentManagers[cID] != nullptr) {
            return *(static_cast<ComponentManager<T>*>(componentManagers[cID]));      
        } else {
            // If not, create one.
            componentManagers[cID] =
                reinterpret_cast<ComponentManager<T>*>(_stackAllocator.
                                allocate(sizeof(ComponentManager<T>), 1));

            size_t paddingBuffer = 8;
            size_t managerMemSize = UINT16_MAX * sizeof(T) +
                                    2 * UINT16_MAX * sizeof(uint16_t)
                                    + paddingBuffer;
            void* managerMemPointer = _stackAllocator.
                                      allocate(managerMemSize, sizeof(T));

            new(componentManagers[cID]) ComponentManager<T>(managerMemPointer, managerMemSize);
            
            return *(static_cast<ComponentManager<T>*>(componentManagers[cID]));      
        }
    }
};

#endif