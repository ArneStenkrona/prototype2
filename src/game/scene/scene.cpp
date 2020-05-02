#include "scene.h"

#include "src/graphics/geometry/parametric_shapes.h"

prt::vector<glm::vec3> tris{};
PhysicsSystem physicsSystem{};

Scene::Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem, 
             Input& input, Camera& camera)
    : m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
      m_camera(camera),
      m_gravityConstant({0.0f,-1.0f,0.0f}),
      m_gravity(1.0f) {
    resetTransforms();

    uint32_t islandID = m_assetManager.getModelManager().getModelID("island/island.obj");

    m_staticSolidEntities.modelIDs[0] = islandID;
    m_staticSolidEntities.transforms[0].position = { 0.0f, -20.0f, 0.0f };
    m_staticSolidEntities.size = 1;

    m_lights.sun = { glm::normalize(glm::vec3{0.0f, -1.0f, -1.0f}), {1.0f, 1.0f, 1.0f} };

    initPlayer();
    resolveColliderIDs();
}

void Scene::initPlayer() {
    uint32_t sphere_index = m_assetManager.getModelManager().getModelID("monkey/monkey.fbx");

    m_playerEntity.modelID = sphere_index;
    m_playerEntity.acceleration = 1.0f;
    m_playerEntity.friction = 0.1f;
    m_playerEntity.velocity = {0.0f, 0.0f, 0.0f};
    m_playerEntity.gravityVelocity = {0.0f, 0.0f, 0.0f};
    m_playerEntity.ellipsoidColliderID = m_physicsSystem.addEllipsoidCollider({1.0f, 1.0f, 1.0f});
    m_playerEntity.isGrounded = false;

}

void Scene::getModels(prt::vector<Model>& models,
                prt::vector<uint32_t>& modelIndices) const {
    prt::vector<uint32_t> modelIDs;
    getModelIDs(modelIDs);

    m_assetManager.loadSceneModels(modelIDs, models, modelIndices);
}

void Scene::getSkybox(prt::array<Texture, 6>& cubeMap) const {
    m_assetManager.loadCubeMap("default", cubeMap);
}


void Scene::getModelIDs(prt::vector<uint32_t>& modelIDs) const {
    modelIDs.resize(m_staticEntities.size + 
                    m_staticSolidEntities.size +
                    1 /* player */);
    size_t iID = 0;
    for (size_t i = 0; i <  m_staticEntities.size; i++) {
        modelIDs[iID++] = m_staticEntities.modelIDs[i];
    }
    for (size_t i = 0; i <  m_staticSolidEntities.size; i++) {
        modelIDs[iID++] = m_staticSolidEntities.modelIDs[i];
    }
    modelIDs[iID++] = m_playerEntity.modelID;
}

void Scene::resolveColliderIDs() {
    prt::vector<Model> models;
    prt::vector<uint32_t> modelIDs;
    modelIDs.resize(m_staticSolidEntities.size);

    for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
        modelIDs[i] = m_staticSolidEntities.modelIDs[i];
    }
    prt::vector<uint32_t> modelIndices;
    m_assetManager.loadSceneModels(modelIDs, models, modelIndices);
    for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
       m_staticSolidEntities.triangleMeshColliderIDs[i] = m_physicsSystem.addTriangleMeshCollider(models[i]);
    }
}

void Scene::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices) {
    transformMatrices.resize(m_staticEntities.size +
                             m_staticSolidEntities.size + 
                             1 /* player */);

    size_t iMatrix = 0;
    for (size_t i = 0; i < m_staticEntities.size; i++) {
        transformMatrices[iMatrix++] = m_staticEntities.transforms[i].transformMatrix();
    }
    for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
        transformMatrices[iMatrix++] = m_staticSolidEntities.transforms[i].transformMatrix();
    }
    transformMatrices[iMatrix++] = m_playerEntity.transform.transformMatrix();
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < m_staticEntities.size; i++) {
        m_staticEntities.transforms[i] = {};
    }
    for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
        m_staticSolidEntities.transforms[i] = {};
    }
    m_playerEntity.transform = {};
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
    static float t = 0;

    t+=deltaTime;
    updatePlayerInput();
    updatePlayerPhysics(deltaTime);
    updatePhysics(deltaTime);
    updateCamera();
}

void Scene::updatePlayerInput() {
    m_playerEntity.direction = {0.0f, 0.0f, 0.0f};
    if (m_input.getKeyPress(INPUT_KEY::KEY_W)) {
        m_playerEntity.direction += glm::vec3{1.0f,0.0f,0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_S)) {
        m_playerEntity.direction -= glm::vec3{1.0f,0.0f,0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_A)) {
        m_playerEntity.direction -= glm::vec3{0.0f,0.0f,1.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_D)) {
        m_playerEntity.direction += glm::vec3{0.0f,0.0f,1.0f};    
    }
    m_playerEntity.jump = false;
    if (m_playerEntity.isGrounded && m_input.getKeyDown(INPUT_KEY::KEY_SPACE)) {
        m_playerEntity.jump = true;
    }    
}
void Scene::updatePlayerPhysics(float deltaTime) {
    // reference player fields with shorthands
    glm::vec3& dir = m_playerEntity.direction;
    glm::vec3& vel = m_playerEntity.velocity;
    glm::vec3& gVel = m_playerEntity.gravityVelocity;
    bool& grounded = m_playerEntity.isGrounded;
    glm::vec3& groundNormal = m_playerEntity.groundNormal;
    bool& jump = m_playerEntity.jump;
    // if player performed any movement input
    if (glm::length(glm::vec3{dir.x, 0.0f, dir.z}) > 0.001f) {
        // compute look direction
        glm::vec3 cF = m_camera.getFront();
        glm::vec3 cR = m_camera.getRight();
        glm::vec3 lookDir = glm::normalize(dir.x * glm::vec3(cF.x, 0.0f, cF.z) + dir.z * glm::vec3(cR.x, 0.0f, cR.z));
        // rotate player model
        m_playerEntity.transform.rotation = 
                            safeQuatLookAt({0.0f,0.0f,0.0f}, lookDir, 
                            glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
        // compute movement direction
        glm::vec3 moveNormal = m_playerEntity.isGrounded ? m_playerEntity.groundNormal : glm::vec3{0.0f, 1.0f, 0.0f};

        glm::vec3 moveDir = glm::normalize(glm::cross(moveNormal, 
                                glm::cross(lookDir, moveNormal)));
        // add acceleration to velocity
        float acc = m_playerEntity.acceleration * deltaTime;
        vel += moveDir * acc;
    }

    // add friction
    vel *= glm::pow(m_playerEntity.friction, deltaTime);

    float gnDotUp = glm::dot(groundNormal, glm::vec3{0.0f,1.0f,0.0f});
    if (grounded && gnDotUp < 0.72f) {
        float slideFactor = 5.0f;
        gVel += slideFactor * (1.0f - gnDotUp) * deltaTime * glm::normalize(glm::cross(groundNormal, 
                                                glm::cross(glm::vec3{0.0f,-1.0f,0.0f}, groundNormal)));
    } else if (!grounded) {
        float gAcc = m_gravity * deltaTime;
        gVel += glm::vec3{0.0f, -1.0f, 0.0f} * gAcc;
    } else {
        gVel -= m_playerEntity.groundNormal * deltaTime;
    }

    // jump
    if (jump) {
        gVel += 0.5f * glm::vec3{0.0f, 1.0f, 0.0f};
    }
}

void Scene::updatePhysics(float deltaTime) {
    m_physicsSystem.resolveEllipsoidsTriangles(&m_playerEntity.ellipsoidColliderID,
                                               &m_playerEntity.transform,
                                               &m_playerEntity.velocity,
                                               &m_playerEntity.isGrounded,
                                               &m_playerEntity.groundNormal,
                                               1,
                                               m_staticSolidEntities.triangleMeshColliderIDs,
                                               m_staticSolidEntities.transforms,
                                               m_staticSolidEntities.size,
                                               deltaTime);

    // if (m_applyGravity) {
        m_physicsSystem.resolveEllipsoidsTriangles(&m_playerEntity.ellipsoidColliderID,
                                                   &m_playerEntity.transform,
                                                   &m_playerEntity.gravityVelocity,
                                                   &m_playerEntity.isGrounded,
                                                   &m_playerEntity.groundNormal,
                                                   1,
                                                   m_staticSolidEntities.triangleMeshColliderIDs,
                                                   m_staticSolidEntities.transforms,
                                                   m_staticSolidEntities.size,
                                                   deltaTime);   
    // }
}

void Scene::updateCamera() {
    m_camera.setTarget(m_playerEntity.transform.position);
}