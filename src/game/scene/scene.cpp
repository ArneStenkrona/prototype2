#include "scene.h"

Scene::Scene(AssetManager & assetManager, PhysicsSystem & physicsSystem, 
             Input & input, Camera & camera)
    : m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
      m_camera(camera),
      m_playerSystem(m_input, camera, physicsSystem) {
    resetTransforms();

    uint32_t islandID; 
    char const *islandStr = "docks/docks.dae";
    m_assetManager.loadModels(&islandStr, 1, &islandID, false);

    m_staticSolidEntities.modelIDs[0] = islandID;
    m_staticSolidEntities.transforms[0].position = { 0.0f, -50.0f, 0.0f };
    m_staticSolidEntities.transforms[0].scale = { 1.5f, 1.5f, 1.5f };
    m_staticSolidEntities.size = 1;

    m_lights.sun = { glm::normalize(glm::vec3{0.0f, -1.0f, -1.0f}), {1.0f, 1.0f, 1.0f} };

    initPlayer();
    initColliders();
}

void Scene::bindToRenderer(GameRenderer & gameRenderer) {
    prt::vector<uint32_t> modelIDs;
    Model const * models;
    size_t nModels = 0;
    getNonAnimatedModels(models, nModels, modelIDs);

    prt::vector<uint32_t> animatedModelIDs;
    Model const * animatedModels;
    uint32_t const * boneOffsets;
    size_t nAnimatedModels;
    getAnimatedModels(animatedModels, boneOffsets, nAnimatedModels, animatedModelIDs);


    prt::array<Texture, 6> skybox;
    getSkybox(skybox);

    gameRenderer.bindAssets(models, nModels, modelIDs,
                            animatedModels, boneOffsets, nAnimatedModels, 
                            animatedModelIDs,
                            skybox);
}

void Scene::initPlayer() {
    char const *monkeyStr = "duck/duck.dae";
    m_assetManager.loadModels(&monkeyStr, 1, &m_playerEntity.modelID, true);

    m_playerEntity.transform.scale = {0.6f, 0.6f, 0.6f};
    m_playerEntity.transform.position = {0.6f, -20.0f, 0.6f};

    m_playerEntity.velocity = {0.0f, 0.0f, 0.0f};
    m_playerEntity.gravityVelocity = {0.0f, 0.0f, 0.0f};
    m_playerEntity.ellipsoidColliderID = m_physicsSystem.addEllipsoidCollider({0.5f, 1.0f, 0.5f});
    m_playerEntity.groundNormal = {0.0f, 0.0f, 0.0f};
    m_playerEntity.isGrounded = false;

    m_playerEntity.idleAnimationIndex = m_assetManager.getModelManager().getAnimationIndex(m_playerEntity.modelID, "idle");
    m_playerEntity.walkAnimationIndex = m_assetManager.getModelManager().getAnimationIndex(m_playerEntity.modelID, "walk");
    m_playerEntity.runAnimationIndex = m_assetManager.getModelManager().getAnimationIndex(m_playerEntity.modelID, "run");

}

void Scene::getSkybox(prt::array<Texture, 6> & cubeMap) const {
    m_assetManager.loadCubeMap("default", cubeMap);
}

void Scene::getModelIDs(prt::vector<uint32_t> & modelIDs, bool animated) const {
    if (animated) {
        modelIDs.resize(1);
        modelIDs[0] = m_playerEntity.modelID;
    } else {
        modelIDs.resize(m_staticEntities.size + 
                        m_staticSolidEntities.size);
        size_t iID = 0;
        for (size_t i = 0; i <  m_staticEntities.size; ++i) {
            modelIDs[iID++] = m_staticEntities.modelIDs[i];
        }
        for (size_t i = 0; i <  m_staticSolidEntities.size; ++i) {
            modelIDs[iID++] = m_staticSolidEntities.modelIDs[i];
        }
    }
}

void Scene::initColliders() {
    m_physicsSystem.addModelColliders(m_staticSolidEntities.modelIDs, m_staticSolidEntities.transforms,
                                      m_staticSolidEntities.size,
                                      m_staticSolidEntities.colliderIDs);
}

void Scene::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices, bool animated) const {
    if (animated) {
        transformMatrices.resize(1);
        transformMatrices[0] = m_playerEntity.transform.transformMatrix();
    } else {
        transformMatrices.resize(m_staticEntities.size +
                                 m_staticSolidEntities.size);
        size_t iMatrix = 0;
        for (size_t i = 0; i < m_staticEntities.size; i++) {
            transformMatrices[iMatrix++] = m_staticEntities.transforms[i].transformMatrix();
        }
        for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
            transformMatrices[iMatrix++] = m_staticSolidEntities.transforms[i].transformMatrix();
        }
    }
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

void Scene::update(float deltaTime) {
    static float t = 0;

    t+=deltaTime;
    m_playerSystem.updatePlayer(m_playerEntity, deltaTime);
    updatePhysics();
    updateCamera();
}

void Scene::getNonAnimatedModels(Model const * & models, size_t & nModels,
                      prt::vector<uint32_t> & modelIDs) const {
    m_assetManager.getModelManager().getNonAnimatedModels(models, nModels);
    getModelIDs(modelIDs, false);
}

void Scene::getAnimatedModels(Model const * & models, uint32_t const * & boneOffsets,
                              size_t & nModels, prt::vector<uint32_t> & modelIDs) {
    m_assetManager.getModelManager().getAnimatedModels(models, boneOffsets, nModels);
    getModelIDs(modelIDs, true);
}

void Scene::getSampledAnimation(float /*t*/, prt::vector<glm::mat4> & transforms) {
    prt::vector<uint32_t> modelIndices;
    prt::vector<ModelManager::AnimationBlend> animationBlends;

    modelIndices.push_back(m_playerEntity.modelID);
    animationBlends.push_back({m_playerEntity.animationTimer,
                               m_playerEntity.animationA,
                               m_playerEntity.animationB,
                               m_playerEntity.animationBlendFactor});
    m_assetManager.getModelManager().getSampledBlendedAnimation(modelIndices,
                                                                animationBlends,
                                                                transforms);
}

void Scene::updatePhysics() {
    m_physicsSystem.updateCharacterPhysics(&m_playerEntity.ellipsoidColliderID,
                                           &m_playerEntity.transform.position,
                                           &m_playerEntity.velocity,
                                           &m_playerEntity.gravityVelocity,
                                           &m_playerEntity.groundNormal,
                                           &m_playerEntity.isGrounded,
                                           1);
}

void Scene::updateCamera() {
    glm::vec3 hit;
    glm::vec3 corners[4];
    float dist = 5.0f;
    m_camera.getCameraCorners(corners[0], corners[1], corners[2], corners[3]);
    for (size_t i = 0; i < 4; ++i) {
        glm::vec3 dir = glm::normalize(corners[i] - m_playerEntity.transform.position);
        if (m_physicsSystem.raycast(m_playerEntity.transform.position, dir, 
                                    5.0f, hit)) {
            dist = std::min(dist, glm::distance(m_playerEntity.transform.position, hit));

        }
    }
    m_camera.setTargetDistance(dist);
    m_camera.setTarget(m_playerEntity.transform.position);
}