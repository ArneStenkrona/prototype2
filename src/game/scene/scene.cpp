#include "scene.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/game/system/physics_system.h"

prt::vector<glm::vec3> tris{};
PhysicsSystem physicsSystem{};

Scene::Scene(AssetManager &assetManager, Input& input, Camera& camera)
    :_assetManager(assetManager),
     _input(input),
     _camera(camera) {
    resetTransforms();

    uint32_t monkey_ID = _assetManager.getModelManager().getModelID("tree");
    uint32_t plane_ID = _assetManager.getModelManager().getModelID("plane");


    for (size_t i = 0; i < _staticEntities.maxSize; i++) {
        _staticEntities.modelIDs[i] = monkey_ID;
        _staticEntities.transforms[i].position = { 10.0f * i, 0.0f, 0.0f };
        _staticEntities.transforms[i].rotation = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _staticEntities.transforms[i].scale = glm::vec3{1.0f,1.0f,1.0f};
    }
    _staticEntities.size = _staticEntities.maxSize;

    _staticEntities.modelIDs[_staticEntities.maxSize - 1] = plane_ID;
    _staticEntities.transforms[_staticEntities.maxSize - 1].position = { 0, -10.0f, 0.0f };
    _staticEntities.transforms[_staticEntities.maxSize - 1].scale = { 1.0f, 1.0f, 1.0f };

    initPlayer();

    // temp collision stuff
    prt::vector<uint32_t> modelIDs = { monkey_ID };
    prt::vector<uint32_t> modelIndices{};
    prt::vector<Model> models;
    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
    Model& model = models[0];
    tris.resize(model._indexBuffer.size());
    for (size_t i = 0; i < model._indexBuffer.size(); i++) {
        tris[i] = model._vertexBuffer[model._indexBuffer[i]].pos;
    }
}

void Scene::initPlayer() {
    uint32_t sphere_index = _assetManager.getModelManager().getModelID("sphere");

    //_playerEntity.entityID = RESERVED_ENTITY_IDS::PLAYER_ID;
    _playerEntity.modelID = sphere_index;
    _playerEntity.acceleration = 1.0f;
    _playerEntity.friction = 0.9f;
    _playerEntity.velocity = {0.0f, 0.0f, 0.0f};

}

void Scene::getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices) {
    prt::vector<uint32_t> modelIDs;
    getModelIDs(modelIDs);

    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
}

void Scene::getSkybox(prt::array<Texture, 6>& cubeMap) {
    _assetManager.loadCubeMap("default", cubeMap);
}


void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(_staticEntities.size + 
                    _staticSolidEntities.size +
                    1 /* player */);
    size_t iID = 0;
    for (size_t i = 0; i <  _staticEntities.size; i++) {
        modelIDs[iID++] = _staticEntities.modelIDs[i];
    }
    for (size_t i = 0; i <  _staticSolidEntities.size; i++) {
        modelIDs[iID++] = _staticSolidEntities.modelIDs[i];
    }
    modelIDs[iID++] = _playerEntity.modelID;
}

void Scene::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(_staticEntities.size +
                             _staticSolidEntities.size + 
                             1 /* player */);

    size_t iMatrix = 0;
    for (size_t i = 0; i < _staticEntities.size; i++) {
        transformMatrices[iMatrix++] = _staticEntities.transforms[i].transformMatrix();
    }
    for (size_t i = 0; i < _staticSolidEntities.size; i++) {
        transformMatrices[iMatrix++] = _staticSolidEntities.transforms[i].transformMatrix();
    }
    transformMatrices[iMatrix++] = _playerEntity.transform.transformMatrix();
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < _staticEntities.size; i++) {
        _staticEntities.transforms[i] = {};
    }
    for (size_t i = 0; i < _staticSolidEntities.size; i++) {
        _staticSolidEntities.transforms[i] = {};
    }
    _playerEntity.transform = {};
}

glm::quat safeQuatLookAt(
    glm::vec3 const& lookFrom,
    glm::vec3 const& lookTo,
    glm::vec3 const& up,
    glm::vec3 const& alternativeUp)
{
    glm::vec3  direction       = lookTo - lookFrom;
    float      directionLength = glm::length(direction);

    // Check if the direction is valid; Also deals with NaN
    if(!(directionLength > 0.0001))
        return glm::quat(1, 0, 0, 0); // Just return identity

    // Normalize direction
    direction /= directionLength;

    // Is the normal up (nearly) parallel to direction?
    if(glm::abs(glm::dot(direction, up)) > .9999f) {
        // Use alternative up
        return glm::quatLookAt(direction, alternativeUp);
    }
    else {
        return glm::quatLookAt(direction, up);
    }
}

void Scene::update(float deltaTime) {
    updatePlayer(deltaTime);
}

void Scene::updatePlayer(float deltaTime) {
    float acc = _playerEntity.acceleration * deltaTime;
    glm::vec3 dir = {0.0f,0.0f,0.0f};
    float eps = 0.0001f;
    if (_input.getKey(GLFW_KEY_W) == GLFW_PRESS) {
        glm::vec3 front = _camera.getFront();
        dir += glm::normalize(glm::vec3{front.x, 0.0f, front.z});
    }
    if (_input.getKey(GLFW_KEY_S) == GLFW_PRESS) {
        glm::vec3 front = _camera.getFront();
        dir -= glm::normalize(glm::vec3{front.x, 0.0f, front.z});
    }
    if (_input.getKey(GLFW_KEY_A) == GLFW_PRESS) {
        dir -= glm::normalize(glm::cross(_camera.getFront(), _camera.getUp()));
    }
    if (_input.getKey(GLFW_KEY_D) == GLFW_PRESS) {
        dir += glm::normalize(glm::cross(_camera.getFront(), _camera.getUp()));
    }
    if (_input.getKey(GLFW_KEY_SPACE) == GLFW_PRESS) {
        dir += glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (_input.getKey(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        dir -= glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3& pos = _playerEntity.transform.position;
    glm::vec3& vel = _playerEntity.velocity;
    if (glm::length(dir) > eps) {
        vel += glm::normalize(dir) * acc;
            
    }
    glm::vec3 lookDir = glm::normalize(glm::vec3{-dir.x, 0.0f, -dir.z});
    if (glm::length(lookDir) > eps) { 
        _playerEntity.transform.rotation = 
                            safeQuatLookAt({0.0f,0.0f,0.0f}, lookDir, 
                            glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
    }

    vel *= _playerEntity.friction;

    // temp collision stuff
    // glm::vec3 ellipsoid = { 1.0f, 1.0f, 1.0f };
    // glm::vec3 intersectionPoint;
    // float intersectionDistance;
    // physicsSystem.collideAndRespondEllipsoidTriangles(ellipsoid, pos, vel, 
    //                                                   tris, _entities.positions[2],
    //                                                   glm::vec3{0.0f,0.0f,0.0f},
    //                                                   intersectionPoint,
    //                                                   intersectionDistance);

    _camera.setTarget(pos);
    pos += vel;
}