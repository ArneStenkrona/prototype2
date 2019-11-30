#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Scene::Scene(ModelManager &modelManager)
    :_modelManager(modelManager) {
    resetTransforms();

    uint32_t default_index = ModelManager::DEFAULT_MODEL;
    uint32_t monkey_index = _modelManager.getModelID("MONKEY");
    uint32_t sphere_index = _modelManager.getModelID("SPHERE");
    uint32_t plane_index = _modelManager.getModelID("PLANE");

    uint32_t indices[3] = {default_index, monkey_index, sphere_index};

    _staticEntities.modelIDs[0] = plane_index;
    _staticEntities.positions[0] = { 0, -10.0f, 0.0f };
    _staticEntities.scales[0] = { 1.0f, 1.0f, 1.0f };

    for (size_t i = 1; i < MAXIMUM_STATIC_ENTITIES; i++) {
        _staticEntities.modelIDs[i] = indices[i % 3];
        _staticEntities.positions[i] = { 10.0f * i, 0.0f, 0.0f };
        _staticEntities.rotations[i] =  glm::quat(glm::vec3{i*22.5f, -i*22.5f, 0.0f});
        _staticEntities.scales[i] = { ((i + 1) / 3.0f) * 1.0f, ((i + 1) / 5.0f) * 1.0f, ((i + 1) / 1.0f) * 1.0f };
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

void Scene::resetTransforms() {
    for (size_t i = 0; i < MAXIMUM_STATIC_ENTITIES; i++) {
        _staticEntities.positions[i] = { 0.0f, 0.0f, 0.0f };
        _staticEntities.rotations[i] =  glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _staticEntities.scales[i] = { 1.0f, 1.0f, 1.0f };
    }
}