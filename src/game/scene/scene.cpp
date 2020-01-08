#include "scene.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/game/system/physics_system.h"

prt::vector<glm::vec3> tris{};
PhysicsSystem physicsSystem{};

Scene::Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem, 
             Input& input, Camera& camera)
    :_assetManager(assetManager),
     _physicsSystem(physicsSystem),
     _input(input),
     _camera(camera),
     _gravityConstant({0.0f,-1.0f,0.0f}) {
    resetTransforms();

    uint32_t tree_ID = _assetManager.getModelManager().getModelID("tree");
    uint32_t plane_ID = _assetManager.getModelManager().getModelID("little_place");


    for (size_t i = 0; i < _staticEntities.maxSize; i++) {
        _staticEntities.modelIDs[i] = tree_ID;
        _staticEntities.transforms[i].position = { 10.0f * i, 0.0f, 0.0f };
        _staticEntities.transforms[i].rotation = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _staticEntities.transforms[i].scale = glm::vec3{1.0f,1.0f,1.0f};
    }
    _staticEntities.size = _staticEntities.maxSize;

    uint32_t colliderMonkey_ID = _assetManager.getModelManager().getModelID("collider_monkey");
    for (size_t i = 0; i < _staticSolidEntities.maxSize; i++) {
        _staticSolidEntities.modelIDs[i] = colliderMonkey_ID;
        _staticSolidEntities.transforms[i].position = { 10.0f * i, 5.0f, 0.0f };
        _staticSolidEntities.transforms[i].rotation = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _staticSolidEntities.transforms[i].scale = glm::vec3{1.0f,1.0f,1.0f};
    }
    _staticSolidEntities.size = _staticSolidEntities.maxSize;

    _staticSolidEntities.modelIDs[_staticSolidEntities.maxSize - 1] = plane_ID;
    _staticSolidEntities.transforms[_staticSolidEntities.maxSize - 1].position = { 0, -10.0f, 0.0f };
    _staticSolidEntities.transforms[_staticSolidEntities.maxSize - 1].scale = { 1.0f, 1.0f, 1.0f };

    initPlayer();
}

void Scene::initPlayer() {
    uint32_t sphere_index = _assetManager.getModelManager().getModelID("sphere");

    _playerEntity.modelID = sphere_index;
    _playerEntity.acceleration = 1.0f;
    _playerEntity.friction = 0.9f;
    _playerEntity.velocity = {0.0f, 0.0f, 0.0f};
    _playerEntity.ellipsoidColliderID = _physicsSystem.addEllipsoidCollider({1.0f, 1.0f, 1.0f});

}

void Scene::load(VulkanApplication& vulkanApplication) {
    prt::vector<uint32_t> modelIDs;
    getModelIDs(modelIDs);

    prt::vector<Model> models;
    prt::vector<uint32_t> modelIndices;

    prt::vector<Model> colliderModels;
    prt::vector<uint32_t> colliderIDs;


    _assetManager.loadSceneModels(modelIDs, models, modelIndices,
                                  colliderModels, colliderIDs);

    _physicsSystem.loadTriangleMeshColliders(colliderModels, colliderIDs);
    resolveColliderIDs();

    prt::array<Texture, 6> skybox;
    getSkybox(skybox);
    vulkanApplication.bindScene(models, modelIndices, skybox);
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

void Scene::resolveColliderIDs() {
    for (size_t i = 0; i < _staticSolidEntities.maxSize; i++) {
        uint32_t mID = _staticSolidEntities.modelIDs[i];
        _staticSolidEntities.triangleMeshColliderIDs[i] = _physicsSystem.getTriangleMeshID(mID);
    }
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
    updatePhysics(deltaTime);
    updateCamera();
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

    if (_input.getKey(GLFW_KEY_G) == GLFW_PRESS) {
        _applyGravity = !_applyGravity;
    }

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
    if (_applyGravity) {
        _playerEntity.gravityVelocity += _gravityConstant * deltaTime;
    } else {
        _playerEntity.gravityVelocity = {0.0f,0.0f,0.0f};
    }
}

void Scene::updatePhysics(float /*deltaTime*/) {
    _physicsSystem.resolveEllipsoidsTriangles(&_playerEntity.ellipsoidColliderID,
                                              &_playerEntity.transform,
                                              &_playerEntity.velocity,
                                              1,
                                              _staticSolidEntities.triangleMeshColliderIDs,
                                              _staticSolidEntities.transforms,
                                              _staticSolidEntities.size);

    if (_applyGravity) {
        _physicsSystem.resolveEllipsoidsTriangles(&_playerEntity.ellipsoidColliderID,
                                                &_playerEntity.transform,
                                                &_playerEntity.gravityVelocity,
                                                1,
                                                _staticSolidEntities.triangleMeshColliderIDs,
                                                _staticSolidEntities.transforms,
                                                _staticSolidEntities.size);   
    }
}

void Scene::updateCamera() {
    _camera.setTarget(_playerEntity.transform.position);
}