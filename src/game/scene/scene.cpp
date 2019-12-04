#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Scene::Scene(AssetManager &assetManager)
    :_assetManager(assetManager) {
    resetTransforms();

    uint32_t default_index = ModelManager::DEFAULT_MODEL;
    uint32_t monkey_index = _assetManager.getModelManager().getModelID("MONKEY");
    uint32_t sphere_index = _assetManager.getModelManager().getModelID("SPHERE");
    uint32_t plane_index = _assetManager.getModelManager().getModelID("PLANE");
    uint32_t quad_index = _assetManager.getModelManager().getModelID("CYLINDER0");

    uint32_t indices[4] = {default_index, monkey_index, sphere_index, quad_index};

    _modelEntities.modelIDs[0] = plane_index;
    _modelEntities.positions[0] = { 0, -10.0f, 0.0f };
    _modelEntities.scales[0] = { 1.0f, 1.0f, 1.0f };

    for (size_t i = 1; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _modelEntities.modelIDs[i] = indices[3];//indices[i % 4];
        _modelEntities.positions[i] = { 40.0f * i, 0.0f, 0.0f };
        _modelEntities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});//glm::quat(glm::vec3{i*22.5f, -i*22.5f, 0.0f});
        _modelEntities.scales[i] = glm::vec3{1.0f,1.0f,1.0f};//{ ((i + 1) / 3.0f) * 1.0f, ((i + 1) / 5.0f) * 1.0f, ((i + 1) / 1.0f) * 1.0f };
    }
}

void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(MAXIMUM_MODEL_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        modelIDs[i] = _modelEntities.modelIDs[i];
    }
}

void Scene::getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(MAXIMUM_MODEL_ENTITIES);
    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        glm::mat4 scale = glm::scale(_modelEntities.scales[i]);
        glm::mat4 rotate = glm::toMat4(_modelEntities.rotations[i]);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), _modelEntities.positions[i]);
        transformMatrices[i] = translate * rotate * scale;
    }
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _modelEntities.positions[i] = { 0.0f, 0.0f, 0.0f };
        _modelEntities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _modelEntities.scales[i] = { 1.0f, 1.0f, 1.0f };
    }
}