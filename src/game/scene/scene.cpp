#include "scene.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/game/system/physics_system.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

prt::vector<glm::vec3> tris{};
PhysicsSystem physicsSystem{};

Scene::Scene(AssetManager &assetManager, Input& input, Camera& camera)
    :_assetManager(assetManager),
     _input(input),
     _camera(camera) {
    resetTransforms();

    uint32_t monkey_ID = _assetManager.getModelManager().getModelID("monkey.obj");
    uint32_t plane_ID = _assetManager.getModelManager().getModelID("plane.obj");


    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _entities.modelIDs[i] = monkey_ID;
        _entities.positions[i] = { 10.0f * i, 0.0f, 0.0f };
        _entities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _entities.scales[i] = glm::vec3{1.0f,1.0f,1.0f};
    }
        _entities.numEntities = MAXIMUM_MODEL_ENTITIES;

    _entities.modelIDs[MAXIMUM_MODEL_ENTITIES - 1] = plane_ID;
    _entities.positions[MAXIMUM_MODEL_ENTITIES - 1] = { 0, -10.0f, 0.0f };
    _entities.scales[MAXIMUM_MODEL_ENTITIES - 1] = { 1.0f, 1.0f, 1.0f };

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
    uint32_t sphere_index = _assetManager.getModelManager().getModelID("sphere.obj");

    _player.entityID = RESERVED_ENTITY_IDS::PLAYER_ID;
    _player.acceleration = 1.0f;
    _player.friction = 0.9f;
    _player.velocity = {0.0f, 0.0f, 0.0f};

    _entities.modelIDs[_player.entityID] = sphere_index;
}

void Scene::getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices) {
    prt::vector<uint32_t> modelIDs;
    modelIDs.resize( _entities.numEntities);
    for (size_t i = 0; i <  _entities.numEntities; i++) {
        modelIDs[i] = _entities.modelIDs[i];
    }
    _assetManager.loadSceneModels(modelIDs, models, modelIndices);
}

void Scene::getSkybox(prt::array<Texture, 6>& cubeMap) {
    _assetManager.loadCubeMap("default", cubeMap);
}


void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) {
    modelIDs.resize(_entities.numEntities);
    for (size_t i = 0; i < _entities.numEntities; i++) {
        modelIDs[i] = _entities.modelIDs[i];
    }
}

void Scene::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(_entities.numEntities);
    for (size_t i = 0; i < _entities.numEntities; i++) {
        glm::mat4 scale = glm::scale(_entities.scales[i]);
        glm::mat4 rotate = glm::toMat4(_entities.rotations[i]);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), _entities.positions[i]);
        transformMatrices[i] = translate * rotate * scale;
    }
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < MAXIMUM_MODEL_ENTITIES; i++) {
        _entities.positions[i] = { 0.0f, 0.0f, 0.0f };
        _entities.rotations[i] = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});
        _entities.scales[i] = { 1.0f, 1.0f, 1.0f };
    }
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
    float acc = _player.acceleration * deltaTime;
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

    glm::vec3& pos = _entities.positions[_player.entityID];
    glm::vec3& vel = _player.velocity;
    if (glm::length(dir) > eps) {
        vel += glm::normalize(dir) * acc;
            
    }
    glm::vec3 lookDir = glm::normalize(glm::vec3{-dir.x, 0.0f, -dir.z});
    if (glm::length(lookDir) > eps) { 
        _entities.rotations[_player.entityID] = 
                            safeQuatLookAt({0.0f,0.0f,0.0f}, lookDir, 
                             glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
    }

    vel *= _player.friction;

    // temp collision stuff
    glm::vec3 ellipsoid = { 1.0f, 1.0f, 1.0f };
    glm::vec3 intersectionPoint;
    float intersectionDistance;
    physicsSystem.collideAndRespondEllipsoidTriangles(ellipsoid, pos, vel, 
                                                      tris, _entities.positions[2],
                                                      glm::vec3{0.0f,0.0f,0.0f},
                                                      intersectionPoint,
                                                      intersectionDistance);

    _camera.setTarget(pos);
    //pos += vel;
}