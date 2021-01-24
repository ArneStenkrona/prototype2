#include "scene.h"

#include "src/util/io_util.h"

Scene::Scene(GameRenderer & gameRenderer, AssetManager & assetManager, PhysicsSystem & physicsSystem, 
             Input & input)
    : m_gameRenderer(gameRenderer),
      m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
      m_camera(m_input),
      m_characterSystem(this, m_physicsSystem, m_animationSystem) {
    SceneSerialization::loadScene((m_assetManager.getDirectory() + "scenes/docks.prt").c_str(), *this);
    initSky();
}

void Scene::bindToRenderer() {
    prt::array<Texture, 6> skybox;
    getSkybox(skybox);

    bindRenderData();
    
    m_gameRenderer.bindAssets(m_renderData.models,
                              m_renderData.nModels,
                              m_renderData.staticModelIDs.data(), m_renderData.staticEntityIDs.data(),
                              m_renderData.staticModelIDs.size(),
                              m_renderData.animatedModelIDs.data(), m_renderData.animatedEntityIDs.data(),
                              m_renderData.boneOffsets.data(),
                              m_renderData.animatedModelIDs.size(),
                              &m_moon.billboard, 1,
                              m_renderData.textures, m_renderData.nTextures,
                              skybox);
}

void Scene::bindRenderData() {
    // clear previouus render data
    m_renderData.staticTransforms.resize(0);
    m_renderData.staticEntityIDs.resize(0);
    m_renderData.staticModelIDs.resize(0);
    m_renderData.animatedTransforms.resize(0);
    m_renderData.animatedEntityIDs.resize(0);
    m_renderData.animatedModelIDs.resize(0);
    m_renderData.boneOffsets.resize(0);

    m_assetManager.getModelManager().getModels(m_renderData.models, m_renderData.nModels);
    Model const * models = m_renderData.models;

    for (EntityID i = 0;  i < m_entities.size(); ++i) {
        ModelID mid = m_entities.modelIDs[i];

        if (mid != -1) {
            if (models[mid].isAnimated()) {
                m_renderData.animatedModelIDs.push_back(mid);
                m_renderData.animatedEntityIDs.push_back(i);
            } else {
                m_renderData.staticModelIDs.push_back(mid);
                m_renderData.staticEntityIDs.push_back(i);
            }
        }
    }

    m_renderData.staticTransforms.resize(m_renderData.staticModelIDs.size());
    m_renderData.animatedTransforms.resize(m_renderData.animatedModelIDs.size());

    m_renderData.boneOffsets.resize(m_renderData.animatedModelIDs.size());
    m_assetManager.getModelManager().getBoneOffsets(m_renderData.animatedModelIDs.data(),
                                                    m_renderData.boneOffsets.data(),
                                                    m_renderData.animatedModelIDs.size());

    m_assetManager.getTextureManager().getTextures(m_renderData.textures, m_renderData.nTextures);
}

void Scene::renderScene(Camera & camera) {
    updateRenderData();

    prt::vector<glm::mat4> bones; 
    sampleAnimation(bones);

    prt::vector<glm::vec4> billboardPositions = { m_moon.position };
    prt::vector<glm::vec4> billboardColors = { m_moon.billboard.color };

    prt::vector<PointLight> pointLights = getPointLights();
    prt::vector<PackedBoxLight> boxLights = getBoxLights();

    double x,y;
    m_input.getCursorPos(x,y);
    m_renderResult = m_gameRenderer.update(m_renderData.staticTransforms, 
                                           m_renderData.animatedTransforms,
                                           bones,
                                           billboardPositions,
                                           billboardColors,
                                           camera, 
                                           m_lights.sun,
                                           pointLights,
                                           boxLights,
                                           time,
                                           glm::vec2{x,y});
}

void Scene::getSkybox(prt::array<Texture, 6> & cubeMap) const {
    m_assetManager.loadCubeMap("stars", cubeMap);
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

void Scene::update(float deltaTime) {
    updateModels();
    time+=deltaTime;
    updateSun(time);
    m_characterSystem.updateCharacters(deltaTime);
    updatePhysics(deltaTime);
    updateCamera(deltaTime);
    renderScene(m_camera);
}

void Scene::sampleAnimation(prt::vector<glm::mat4> & bones) {
    BlendedAnimation * blends;
    size_t nBlends;

    m_animationSystem.getAnimationBlends(blends, nBlends);

    m_assetManager.getModelManager().getSampledBlendedAnimation(m_renderData.animatedModelIDs.data(),
                                                                blends,
                                                                bones,
                                                                m_renderData.animatedModelIDs.size());
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
    updateColliders();
    m_characterSystem.updatePhysics(deltaTime);
}

void Scene::updateColliders() {
    prt::vector<ColliderTag> tags;
    prt::vector<Transform> transforms;

    for (auto it = m_colliderUpdateSet.begin(); it != m_colliderUpdateSet.end(); it++) {
        ColliderTag tag =  m_entities.colliderTags[it->value()];
        if (tag.type != ColliderType::COLLIDER_TYPE_MODEL) continue;
        tags.push_back(tag);
        transforms.push_back(m_entities.transforms[it->value()]);
    }

    m_physicsSystem.updateModelColliders(tags.data(),  transforms.data(), tags.size());

    m_colliderUpdateSet = prt::hash_set<EntityID>();
}

void Scene::updateModels() {
    if (m_updateModels) {
        bindToRenderer();
        m_updateModels = false;
    }
}

void Scene::updateCamera(float deltaTime) {
    m_camera.update(deltaTime);
    
    // Make sure player is visible
    glm::vec3 hit;
    glm::vec3 corners[4];
    float dist = 5.0f;
    m_camera.getCameraCorners(corners[0], corners[1], corners[2], corners[3]);

    EntityID playerID = m_characterSystem.getPlayer();

    auto const & transform = m_entities.transforms[playerID];
    for (size_t i = 0; i < 4; ++i) {
        glm::vec3 dir = glm::normalize(corners[i] - transform.position);
        if (m_physicsSystem.raycast(transform.position, dir, 
                                    5.0f, hit)) {
            dist = std::min(dist, glm::distance(transform.position, hit));

        }
    }
    m_camera.setTargetDistance(dist);
    m_camera.setTarget(transform.position);

    // move camera light to camera
    m_lights.pointLights[0].pos = m_camera.getPosition();
}

void Scene::updateRenderData() {
    Model const * models = m_renderData.models;

    size_t staticCount = 0;
    size_t animatedCount = 0;

    for (EntityID i = 0;  i < m_entities.size(); ++i) {
        ModelID mid = m_entities.modelIDs[i];

        if (mid != -1) {
            glm::mat4 transform = m_entities.transforms[i].transformMatrix();
            if (models[mid].isAnimated()) {
                m_renderData.animatedTransforms[animatedCount] = transform;
                ++animatedCount;
            } else {
                m_renderData.staticTransforms[staticCount] = transform;
                ++staticCount;
            }
        }
    }
}

void Scene::initSky() {
    m_moon.billboard.color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_moon.billboard.size = {25.0f, 25.0f};
    m_moon.billboard.textureIndex = m_assetManager.getTextureManager().loadTexture("moon/moon_albedo.png");
    m_moon.distance = 200.0f;
}

ModelID Scene::loadModel(char const * path, bool loadAnimation, bool isAbsolute) {
    bool alreadyLoaded;

    ModelID id;
    if (isAbsolute) {
        char relative[512] = {0};

        io_util::getRelativePath(m_assetManager.getDirectory().c_str(), path, relative);

        id = m_assetManager.getModelManager().loadModel(relative, loadAnimation, alreadyLoaded);
    } else {
        id = m_assetManager.getModelManager().loadModel(path, loadAnimation, alreadyLoaded);
    }


    if (!alreadyLoaded) {
        m_updateModels = true;
    }

    return id;
}
