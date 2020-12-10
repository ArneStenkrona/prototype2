#include "scene.h"

Scene::Scene(GameRenderer & gameRenderer, AssetManager & assetManager, PhysicsSystem & physicsSystem, 
             Input & input)
    : m_gameRenderer(gameRenderer),
      m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
      m_camera(m_input),
      m_characterSystem(m_input, m_camera, physicsSystem, assetManager) {
    SceneSerialization::loadScene((m_assetManager.getDirectory() + "scenes/docks.prt").c_str(), *this);
    initColliders();
    initSky();
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
    size_t nAnimatedModels;
    getAnimatedModels(animatedModels, nAnimatedModels, animatedModelIDs, nAnimatedModelIDs);

    prt::vector<uint32_t> boneOffsets;
    boneOffsets.resize(nAnimatedModelIDs);
    m_assetManager.getModelManager().getBoneOffsets(animatedModelIDs, boneOffsets.data(), nAnimatedModelIDs);

    prt::array<Texture, 6> skybox;
    getSkybox(skybox);

    Texture const * textures;
    size_t nTextures;
    m_assetManager.getTextureManager().getTextures(textures, nTextures);

    gameRenderer.bindAssets(models, nModels, modelIDs, nModelIDs,
                            animatedModels, boneOffsets.data(), nAnimatedModels, 
                            animatedModelIDs, nAnimatedModelIDs,
                            &m_moon.billboard, 1,
                            textures, nTextures,
                            skybox);
}

void Scene::renderScene(Camera & camera) {
    prt::vector<glm::mat4> modelMatrices; 
    getTransformMatrices(modelMatrices, false);
    prt::vector<glm::mat4> animatedModelMatrices; 
    getTransformMatrices(animatedModelMatrices, true);

    prt::vector<glm::mat4> bones; 
    sampleAnimation(bones);

    prt::vector<glm::vec4> billboardPositions = { m_moon.position };
    prt::vector<glm::vec4> billboardColors = { m_moon.billboard.color };

    prt::vector<PointLight> pointLights = getPointLights();
    prt::vector<PackedBoxLight> boxLights = getBoxLights();
    m_gameRenderer.update(modelMatrices, 
                          animatedModelMatrices,
                          bones,
                          billboardPositions,
                          billboardColors,
                          camera, 
                          m_lights.sun,
                          pointLights,
                          boxLights,
                          time);
}

void Scene::getSkybox(prt::array<Texture, 6> & cubeMap) const {
    m_assetManager.loadCubeMap("stars", cubeMap);
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

struct IndexedDistance {
    unsigned int index;
    float distance;
};
int compLightsToCamera(const void * elem1, const void * elem2) {
    IndexedDistance f = *((IndexedDistance*)elem1);
    IndexedDistance s = *((IndexedDistance*)elem2);
    if (f.distance > s.distance) return  1;
    if (f.distance < s.distance) return -1;
    return 0;
}

prt::vector<PointLight> Scene::getPointLights() {
    prt::vector<PointLight> ret;
    
    prt::vector<IndexedDistance> distances;
    distances.resize(m_lights.pointLights.size());

    for (size_t i = 0; i < distances.size(); ++i) {
        distances[i].index = i;
        distances[i].distance = glm::distance2(m_lights.pointLights[i].pos, m_camera.getPosition());
    }
    // sort lights by distance to camera
    qsort(distances.data(), distances.size(), sizeof(distances[0]), compLightsToCamera);

    size_t size = glm::min(size_t(NUMBER_SUPPORTED_POINTLIGHTS), m_lights.pointLights.size());
    ret.resize(size);
    for (size_t i = 0; i < size; ++i) {
        ret[i] = m_lights.pointLights[distances[i].index];
    }
    return ret;
}

prt::vector<PackedBoxLight> Scene::getBoxLights() {
    prt::vector<PackedBoxLight> ret;
    
    prt::vector<IndexedDistance> distances;
    distances.resize(m_lights.boxLights.size());

    for (size_t i = 0; i < distances.size(); ++i) {
        distances[i].index = i;
        distances[i].distance = glm::distance2(m_lights.boxLights[i].position, m_camera.getPosition());
    }
    // sort lights by distance to camera
    qsort(distances.data(), distances.size(), sizeof(distances[0]), compLightsToCamera);

    size_t size = glm::min(size_t(NUMBER_SUPPORTED_BOXLIGHTS), m_lights.boxLights.size());
    ret.resize(size);
    for (size_t i = 0; i < size; ++i) {
        auto const & l = m_lights.boxLights[distances[i].index];
        ret[i].min = l.min;
        ret[i].max = l.max;
        ret[i].color = l.color;
        ret[i].invtransform = l.inverseTransform();
    }
    return ret; 
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

void Scene::update(float deltaTime) {
    time+=deltaTime;
    updateSun(time);
    m_characterSystem.updateCharacters(deltaTime);
    updatePhysics(deltaTime);
    updateCamera(deltaTime);
    renderScene(m_camera);
}

void Scene::getNonAnimatedModels(Model const * & models, size_t & nModels,
                                 uint32_t const * & modelIDs, size_t & nModelIDs) const {
    m_assetManager.getModelManager().getNonAnimatedModels(models, nModels);
    modelIDs = getModelIDs(nModelIDs, false);
}

void Scene::getAnimatedModels(Model const * & models, size_t & nModels, 
                              uint32_t const * & modelIDs, size_t & nModelIDs) {
    m_assetManager.getModelManager().getAnimatedModels(models, nModels);
    modelIDs = getModelIDs(nModelIDs, true);
}

void Scene::sampleAnimation(prt::vector<glm::mat4> & bones) {
    m_characterSystem.sampleAnimation(bones);
}

void Scene::updateSun(float time) {
    float ph = 0.2f*time;
    m_lights.sun.phase = ph;
    m_lights.sun.direction = glm::normalize(glm::vec3(0, glm::cos(ph), glm::sin(ph)));
    float distToNoon = glm::acos(glm::dot(-m_lights.sun.direction, glm::vec3(0,1,0))) / glm::pi<float>();
    m_lights.sun.color = glm::mix(glm::vec3(255,255,255), glm::vec3(255,153,51), distToNoon)/255.0f;

    m_lights.sun.distToNoon = glm::acos(glm::dot(-m_lights.sun.direction, glm::vec3(0.0f,1.0f,0.0f))) / glm::pi<float>();
    m_lights.sun.nightColor = glm::mix(glm::vec3(80.0f,80.0f,250.0f), glm::vec3(0.0f), m_lights.sun.distToNoon)/255.0f;
    m_lights.sun.dayColor = glm::mix(glm::vec3(204.0f,204.0f,255.0f), glm::vec3(5.0f,5.0f,25.0f), m_lights.sun.distToNoon)/255.0f;
    m_lights.sun.sunEdgeColor = glm::vec3(255.0f,119.0f,51.0f)/255.0f;
    m_lights.sun.sunsetriseColor = glm::vec3(255.0f,119.0f,51.0f)/255.0f;
    m_lights.sun.sunColor = glm::mix(glm::vec3(255.0f,255.0f,230.0f), glm::vec3(255.0f,153.0f,51.0f), m_lights.sun.distToNoon)/255.0f;

    m_moon.position = glm::vec4(m_camera.getPosition() + (m_moon.distance * m_lights.sun.direction), 1.0f);
    m_moon.billboard.color = glm::mix(glm::vec4(m_lights.sun.nightColor, 1.0f), glm::vec4(1.0f), distToNoon);
}


void Scene::updatePhysics(float deltaTime) {
    m_characterSystem.updatePhysics(deltaTime);
}

void Scene::updateCamera(float deltaTime) {
    m_camera.update(deltaTime);
    m_characterSystem.updateCamera();
    // move camera light to camera
    m_lights.pointLights[0].pos = m_camera.getPosition();
}

void Scene::initSky() {
    m_moon.billboard.color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_moon.billboard.size = {25.0f, 25.0f};
    m_moon.billboard.textureIndex = m_assetManager.getTextureManager().loadTexture("moon/moon_albedo.png");
    m_moon.distance = 200.0f;
}

