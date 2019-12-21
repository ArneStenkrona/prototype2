#include "scene.h"

#include "src/game/level/level_map.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Scene::Scene(AssetManager &assetManager)
    :_assetManager(assetManager) {
    resetTransforms();

    //uint32_t default_index = ModelManager::DEFAULT_MODEL;
    uint32_t monkey_index = _assetManager.getModelManager().getModelID("TREE");
    uint32_t plane_index = _assetManager.getModelManager().getModelID("PLANE");

    _modelEntities.modelIDs[0] = plane_index;
    _modelEntities.positions[0] = { 0, -10.0f, 0.0f };
    _modelEntities.scales[0] = { 1.0f, 1.0f, 1.0f };
    _modelEntities.numEntities++;

    for (size_t i = 1; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _modelEntities.modelIDs[i] = monkey_index;
        _modelEntities.positions[i] = { 10.0f * i, 0.0f, 0.0f };
        _modelEntities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _modelEntities.scales[i] = glm::vec3{1.0f,1.0f,1.0f};
        _modelEntities.numEntities++;
    }
}

void Scene::getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices) {
    prt::vector<uint32_t> modelIDs;
    modelIDs.resize( _modelEntities.numEntities);
    for (size_t i = 0; i <  _modelEntities.numEntities; i++) {
        modelIDs[i] = _modelEntities.modelIDs[i];
    }
    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
    
    uint32_t levelIndex = models.size();
    models.push_back(Model());
    LevelMap levelMap((_assetManager.getDirectory() +  "levels/test/").c_str());
    levelMap.loadModel(models[levelIndex]);
    modelIndices.push_back(levelIndex);
}

void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(_modelEntities.numEntities);
    for (size_t i = 0; i < _modelEntities.numEntities; i++) {
        modelIDs[i] = _modelEntities.modelIDs[i];
    }
}

void Scene::getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices) {
    // for model entities
    transformMatrices.resize(_modelEntities.numEntities);
    for (size_t i = 0; i < _modelEntities.numEntities; i++) {
        glm::mat4 scale = glm::scale(_modelEntities.scales[i]);
        glm::mat4 rotate = glm::toMat4(_modelEntities.rotations[i]);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), _modelEntities.positions[i]);
        transformMatrices[i] = translate * rotate * scale;
    }
    // for level model
    transformMatrices.push_back(glm::mat4(1.0f));
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _modelEntities.positions[i] = { 0.0f, 0.0f, 0.0f };
        _modelEntities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _modelEntities.scales[i] = { 1.0f, 1.0f, 1.0f };
    }
}