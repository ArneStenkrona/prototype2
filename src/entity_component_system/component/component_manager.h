#include <cstddef> 

typedef size_t EntityID;

template<class T>
class ComponentManager {
public:
private:
    void addComponent(EntityID entityID);
    void removeComponent(EntityID entityID);

    friend class EntityManager;
};