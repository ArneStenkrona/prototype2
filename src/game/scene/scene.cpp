#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Scene::Scene() {
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        _staticEntities.modelIDs[i] = i % 3;
        _staticEntities.positions[i] = { 10.0f * i, 0.0f, 0.0f };
        _staticEntities.rotations[i] =  glm::quat(glm::vec3{i*22.5f, -i*22.5f, 0.0f});//{ 0.0f, 0.0f, 0.0f, 1.0f };
        _staticEntities.scales[i] = { 1.0f, 1.0f, 1.0f };
    }
}

void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(MAXIMUM_STATIC_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        modelIDs[i] = _staticEntities.modelIDs[i];
    }
}

void Scene::getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(MAXIMUM_STATIC_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        glm::mat4 scale = glm::scale(_staticEntities.scales[i]);
        glm::mat4 rotate = glm::toMat4(_staticEntities.rotations[i]);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), _staticEntities.positions[i]);
        transformMatrices[i] = translate * rotate * scale;
    }
}