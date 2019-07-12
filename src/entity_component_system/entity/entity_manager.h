//#include "entity.h"
#include "src/entity_component_system/component/component_manager.h"

#include "src/container/vector.h"

#include <cstddef> 
#include <cassert>

typedef size_t EntityID;

class EntityManager {
public:
    EntityManager();

    EntityID createEntity();

    template<class T>
    void addComponent(EntityID entityID) {
        // T must be of Component Type
        static_assert(std::is_base_of<Component, T>::value);
        assert(_entities[entityID]);
        ComponentManager<T>& componentManager = getComponentManager<T>();

        componentManager.addComponent(entityID);
    }

    template<class T>
    void removeComponent(EntityID entityID) {
        // T must be of Component Type
        static_assert(std::is_base_of<Component, T>::value);
        ComponentManager<T>& componentManager = getComponentManager<T>();
    
        componentManager.removeComponent(entityID);
    }

    template<class T>
    ComponentManager<T>& getComponentManager() {
        return ComponentManager<T>();
    }

    /**
     * Destroy an entity.
     * Note: as of yet this does not free
     * the entity slot
     * @param entityID
     */
    void destroyEntity(EntityID entityID);
private:
   // next ID to be issued to an entity
    EntityID _nextID;
    
    // Until dynamic memory allocation is properly implemented
    // in prototype2, we will use a constant size buffer for
    // all the entities
    static constexpr size_t ENTITY_BUFFER_SIZE = 1000;
    // boolean array of all entities.
    // true denotes an existing entity,
    // false denotes a non-existing entity.
    bool _entities[ENTITY_BUFFER_SIZE];

    //template<class T>
    //prt::vector< ComponentManager<T> > _componentManagers;
};