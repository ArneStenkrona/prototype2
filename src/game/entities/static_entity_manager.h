#ifndef STATIC_ENTITY_MANAGER_H
#define STATIC_ENTITY_MANAGER_H

#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <glm/glm.hpp>


class StaticEntityManager {
public:
    StaticEntityManager();
    
    void getModelIDs(prt::vector<uint32_t>& modelIDs);
    void getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices);

private:

    struct StaticEntities {
        uint32_t modelIDs[MAXIMUM_STATIC_ENTITIES];
        glm::vec3 positions[MAXIMUM_STATIC_ENTITIES];
    } _staticEntities;
};

#endif