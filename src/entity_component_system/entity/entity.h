#include <cstdint>

class Entity {
    typedef uint32_t EntityID;
    const EntityID m_ID;
    
public:
    Entity();

    inline const EntityID getEntityID() const { return m_ID; }
private:
};