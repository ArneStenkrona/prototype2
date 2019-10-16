#include "static_entity_manager.h"

#include <glm/gtc/matrix_transform.hpp>

StaticEntityManager::StaticEntityManager() {
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        glm::vec3 transform = { 10.0f * i, 0.0f, 0.0f };
        uint32_t id = (i % 2);
        _staticEntities.positions[i] = transform;
        _staticEntities.modelIDs[i] = id;
    }
}

void StaticEntityManager::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(MAXIMUM_STATIC_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        modelIDs[i] = _staticEntities.modelIDs[i];
    }
}
void StaticEntityManager::getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(MAXIMUM_STATIC_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), _staticEntities.positions[i]);
        transformMatrices[i] = translate;
    }
}