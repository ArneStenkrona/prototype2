#include "scene.h"

Scene::Scene(AssetManager & assetManager, PhysicsSystem & physicsSystem, 
             Input & input, Camera & camera)
    : m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
    //   m_camera(camera),
      m_characterSystem(m_input, camera, physicsSystem, assetManager) {
    resetTransforms();

    uint32_t islandID; 
    char const *islandStr = "docks/docks.dae";
    m_assetManager.loadModels(&islandStr, 1, &islandID, false);

    m_staticSolidEntities.modelIDs[0] = islandID;
    m_staticSolidEntities.transforms[0].position = { 0.0f, -50.0f, 0.0f };
    m_staticSolidEntities.transforms[0].scale = { 1.5f, 1.5f, 1.5f };
    m_staticSolidEntities.size = 1;

    m_lights.sun = { glm::normalize(glm::vec3{0.0f, -1.0f, -1.0f}), {1.0f, 1.0f, 1.0f} };

    initColliders();
}

void Scene::bindToRenderer(GameRenderer & gameRenderer) {
    uint32_t const * modelIDs;
    size_t nModelIDs;
    Model const * models;
    size_t nModels = 0;
    getNonAnimatedModels(models, nModels, modelIDs, nModelIDs);

    uint32_t const * animatedModelIDs;
    size_t nAnimatedModelIDs;
    Model const * animatedModels;
    uint32_t const * boneOffsets;
    size_t nAnimatedModels;
    getAnimatedModels(animatedModels, boneOffsets, nAnimatedModels, animatedModelIDs, nAnimatedModelIDs);


    prt::array<Texture, 6> skybox;
    getSkybox(skybox);

    gameRenderer.bindAssets(models, nModels, modelIDs, nModelIDs,
                            animatedModels, boneOffsets, nAnimatedModels, 
                            animatedModelIDs, nAnimatedModelIDs,
                            skybox);
}

void Scene::getSkybox(prt::array<Texture, 6> & cubeMap) const {
    m_assetManager.loadCubeMap("default", cubeMap);
}

uint32_t const * Scene::getModelIDs(size_t & nModelIDs, bool animated) const {
    if (animated) {
        return m_characterSystem.getModelIDs(nModelIDs);
    } else {
        nModelIDs = m_staticSolidEntities.size;
        return m_staticSolidEntities.modelIDs;
    }
}

void Scene::initColliders() {
    m_physicsSystem.addModelColliders(m_staticSolidEntities.modelIDs, m_staticSolidEntities.transforms,
                                      m_staticSolidEntities.size,
                                      m_staticSolidEntities.colliderIDs);
}

void Scene::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices, bool animated) const {
    if (animated) {
        m_characterSystem.getTransformMatrices(transformMatrices);
    } else {
        transformMatrices.resize(m_staticSolidEntities.size);
        size_t iMatrix = 0;
        for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
            transformMatrices[iMatrix++] = m_staticSolidEntities.transforms[i].transformMatrix();
        }
    }
}

void Scene::resetTransforms() {
    for (size_t i = 0; i < m_staticSolidEntities.size; i++) {
        m_staticSolidEntities.transforms[i] = {};
    }
}

void Scene::update(float deltaTime) {
    static float t = 0;

    t+=deltaTime;
    m_characterSystem.updateCharacters(deltaTime);
    updatePhysics();
    updateCamera();
}

void Scene::getNonAnimatedModels(Model const * & models, size_t & nModels,
                                 uint32_t const * & modelIDs, size_t & nModelIDs) const {
    m_assetManager.getModelManager().getNonAnimatedModels(models, nModels);
    modelIDs = getModelIDs(nModelIDs, false);
}

void Scene::getAnimatedModels(Model const * & models, uint32_t const * & boneOffsets,
                              size_t & nModels, uint32_t const * & modelIDs, size_t & nModelIDs) {
    m_assetManager.getModelManager().getAnimatedModels(models, boneOffsets, nModels);
    modelIDs = getModelIDs(nModelIDs, true);
}

void Scene::sampleAnimation(prt::vector<glm::mat4> & bones) {
    m_characterSystem.sampleAnimation(bones);
}

void Scene::updatePhysics() {
    m_characterSystem.updatePhysics();
}

void Scene::updateCamera() {
    m_characterSystem.updateCamera();
}