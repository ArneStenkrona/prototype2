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
     _gravityConstant({0.0f,-1.0f,0.0f}),
     _gravity(1.0f) {
    resetTransforms();

    uint32_t islandID = _assetManager.getModelManager().getModelID("island/island.obj");

    _staticSolidEntities.modelIDs[0] = islandID;
    _staticSolidEntities.transforms[0].position = { 0.0f, -20.0f, 0.0f };
    _staticSolidEntities.size = 1;

    initPlayer();
    resolveColliderIDs();
}

void Scene::initPlayer() {
    uint32_t sphere_index = _assetManager.getModelManager().getModelID("monkey/monkey.obj");

    _playerEntity.modelID = sphere_index;
    _playerEntity.acceleration = 1.0f;
    _playerEntity.friction = 0.1f;
    _playerEntity.velocity = {0.0f, 0.0f, 0.0f};
    _playerEntity.gravityVelocity = {0.0f, 0.0f, 0.0f};
    _playerEntity.ellipsoidColliderID = _physicsSystem.addEllipsoidCollider({1.0f, 1.0f, 1.0f});
    _playerEntity.isGrounded = false;

}

void Scene::getModels(prt::vector<Model>& models,
                prt::vector<uint32_t>& modelIndices) const {
    prt::vector<uint32_t> modelIDs;
    getModelIDs(modelIDs);

    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
}

void Scene::getSkybox(prt::array<Texture, 6>& cubeMap) const {
    _assetManager.loadCubeMap("default", cubeMap);
}


void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) const {
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
    prt::vector<Model> models;
    prt::vector<uint32_t> modelIDs;
    modelIDs.resize(_staticSolidEntities.size);

    for (size_t i = 0; i < _staticSolidEntities.size; i++) {
        modelIDs[i] = _staticSolidEntities.modelIDs[i];
    }
    prt::vector<uint32_t> modelIndices;
    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
    for (size_t i = 0; i < _staticSolidEntities.size; i++) {
       _staticSolidEntities.triangleMeshColliderIDs[i] = _physicsSystem.addTriangleMeshCollider(models[i]);
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
    updatePlayerInput();
    updatePlayerPhysics(deltaTime);
    updatePhysics(deltaTime);
    updateCamera();
}

void Scene::updatePlayerInput() {
    _playerEntity.direction = {0.0f, 0.0f, 0.0f};
    if (_input.getKeyPress(INPUT_KEY::KEY_W)) {
        _playerEntity.direction += glm::vec3{1.0f,0.0f,0.0f};
    }
    if (_input.getKeyPress(INPUT_KEY::KEY_S)) {
        _playerEntity.direction -= glm::vec3{1.0f,0.0f,0.0f};
    }
    if (_input.getKeyPress(INPUT_KEY::KEY_A)) {
        _playerEntity.direction -= glm::vec3{0.0f,0.0f,1.0f};
    }
    if (_input.getKeyPress(INPUT_KEY::KEY_D)) {
        _playerEntity.direction += glm::vec3{0.0f,0.0f,1.0f};    
    }
    _playerEntity.jump = false;
    if (_playerEntity.isGrounded && _input.getKeyDown(INPUT_KEY::KEY_SPACE)) {
        _playerEntity.jump = true;
    }    
}
void Scene::updatePlayerPhysics(float deltaTime) {
    // reference player fields with shorthands
    glm::vec3& dir = _playerEntity.direction;
    glm::vec3& vel = _playerEntity.velocity;
    glm::vec3& gVel = _playerEntity.gravityVelocity;
    bool& grounded = _playerEntity.isGrounded;
    glm::vec3& groundNormal = _playerEntity.groundNormal;
    bool& jump = _playerEntity.jump;
    // if player performed any movement input
    if (glm::length(glm::vec3{dir.x, 0.0f, dir.z}) > 0.001f) {
        // compute look direction
        glm::vec3 cF = _camera.getFront();
        glm::vec3 cR = _camera.getRight();
        glm::vec3 lookDir = glm::normalize(dir.x * glm::vec3(cF.x, 0.0f, cF.z) + dir.z * glm::vec3(cR.x, 0.0f, cR.z));
        // rotate player model
        _playerEntity.transform.rotation = 
                            safeQuatLookAt({0.0f,0.0f,0.0f}, lookDir, 
                            glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
        // compute movement direction
        glm::vec3 moveNormal = _playerEntity.isGrounded ? _playerEntity.groundNormal : glm::vec3{0.0f, 1.0f, 0.0f};

        glm::vec3 moveDir = glm::normalize(glm::cross(moveNormal, 
                                glm::cross(lookDir, moveNormal)));
        // add acceleration to velocity
        float acc = _playerEntity.acceleration * deltaTime;
        vel += moveDir * acc;
    }

    // add friction
    vel *= glm::pow(_playerEntity.friction, deltaTime);

    float gnDotUp = glm::dot(groundNormal, glm::vec3{0.0f,1.0f,0.0f});
    if (grounded && gnDotUp < 0.72f) {
        float slideFactor = 5.0f;
        gVel += slideFactor * (1.0f - gnDotUp) * deltaTime * glm::normalize(glm::cross(groundNormal, 
                                                glm::cross(glm::vec3{0.0f,-1.0f,0.0f}, groundNormal)));
    } else if (!grounded) {
        float gAcc = _gravity * deltaTime;
        gVel += glm::vec3{0.0f, -1.0f, 0.0f} * gAcc;
    } else {
        gVel -= _playerEntity.groundNormal * deltaTime;
    }

    // jump
    if (jump) {
        gVel += 0.5f * glm::vec3{0.0f, 1.0f, 0.0f};
    }
}

void Scene::updatePhysics(float deltaTime) {
    _physicsSystem.resolveEllipsoidsTriangles(&_playerEntity.ellipsoidColliderID,
                                              &_playerEntity.transform,
                                              &_playerEntity.velocity,
                                              &_playerEntity.isGrounded,
                                              &_playerEntity.groundNormal,
                                              1,
                                              _staticSolidEntities.triangleMeshColliderIDs,
                                              _staticSolidEntities.transforms,
                                              _staticSolidEntities.size,
                                              deltaTime);

    // if (_applyGravity) {
        _physicsSystem.resolveEllipsoidsTriangles(&_playerEntity.ellipsoidColliderID,
                                                &_playerEntity.transform,
                                                &_playerEntity.gravityVelocity,
                                                &_playerEntity.isGrounded,
                                                &_playerEntity.groundNormal,
                                                1,
                                                _staticSolidEntities.triangleMeshColliderIDs,
                                                _staticSolidEntities.transforms,
                                                _staticSolidEntities.size,
                                                deltaTime);   
    // }
}

void Scene::updateCamera() {
    _camera.setTarget(_playerEntity.transform.position);
}