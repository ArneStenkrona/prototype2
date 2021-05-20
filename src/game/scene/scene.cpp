#include "scene.h"

#include "src/util/io_util.h"

Scene::Scene(GameRenderer & gameRenderer, AssetManager & assetManager, PhysicsSystem & physicsSystem, 
             Input & input)
    : m_gameRenderer(gameRenderer),
      m_assetManager(assetManager),
      m_physicsSystem(physicsSystem),
      m_input(input),
      m_camera(m_input),
      m_animationSystem(m_assetManager.getModelManager(), *this),
      m_characterSystem(this, m_physicsSystem, m_animationSystem) {
    SceneSerialization::loadScene((m_assetManager.getDirectory() + "scenes/cavern.prt").c_str(), *this);
    loadColliderModels();
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
                              m_renderData.colliderModelIDs.data(),
                              m_renderData.colliderModelIDs.size(),
                              &m_moon.billboard, 1,
                              m_renderData.textures, m_renderData.nTextures,
                              skybox);
}

void Scene::bindRenderData() {
    // clear previous render data
    m_renderData.staticTransforms.resize(0);
    m_renderData.staticEntityIDs.resize(0);
    m_renderData.staticModelIDs.resize(0);
    m_renderData.animatedTransforms.resize(0);
    m_renderData.animatedEntityIDs.resize(0);
    m_renderData.animatedModelIDs.resize(0);
    m_renderData.boneOffsets.resize(0);
    m_renderData.colliderModelIDs.resize(0);
    m_renderData.colliderTransforms.resize(0);

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

    for (EntityID i = 0; i < m_entities.size(); ++i) {
        ColliderTag tag = m_entities.colliderTags[i];

        if (tag.type == COLLIDER_TYPE_ELLIPSOID) {
            EllipsoidCollider const & col = m_physicsSystem.getEllipsoidCollider(tag);
            glm::vec3 pos = m_entities.transforms[i].position + col.offset;
            glm::vec3 scale = col.radii;
            
            glm::mat4 tform = glm::translate(glm::mat4(1.0f), pos) * glm::scale(scale);

            m_renderData.colliderModelIDs.push_back(m_colliderModelIDs.ellipsoid);
            m_renderData.colliderTransforms.push_back(tform);
        }
    }
}

void Scene::renderScene(Camera & camera) {
    updateRenderData();

    prt::vector<glm::mat4> const & bones = m_animationSystem.getBoneTransforms();

    prt::vector<glm::vec4> billboardPositions = { m_moon.position };
    prt::vector<glm::vec4> billboardColors = { m_moon.billboard.color };

    prt::vector<UBOPointLight> pointLights = getPointLights();

    double x,y;
    m_input.getCursorPos(x,y);
    m_renderResult = m_gameRenderer.update(m_renderData.staticTransforms, 
                                           m_renderData.animatedTransforms,
                                           m_renderData.colliderTransforms,
                                           bones,
                                           billboardPositions,
                                           billboardColors,
                                           camera, 
                                           m_lights.sun,
                                           pointLights,
                                           time,
                                           glm::vec2{x,y});
}

void Scene::getSkybox(prt::array<Texture, 6> & cubeMap) const {
    m_assetManager.loadCubeMap("stars", cubeMap);
}

prt::vector<UBOPointLight> Scene::getPointLights() {
    return m_lightingSystem.getNearestPointLights(m_camera, m_entities.transforms);
}

void Scene::update(float deltaTime) {
    updateModels();
    time+=deltaTime;
    updateSun(time);
    m_characterSystem.updateCharacters(deltaTime);
    m_animationSystem.updateAnimation(deltaTime, m_renderData.animatedModelIDs.data(), m_renderData.animatedModelIDs.size());
    updatePhysics(deltaTime);
    updateCamera(deltaTime);
    renderScene(m_camera);
}

void Scene::updateSun(float time) {
    float ph = 0.2f*time;
    m_lights.sun.phase = ph;
    m_lights.sun.direction = glm::normalize(glm::vec3(0.2f, glm::cos(ph), glm::sin(ph)));
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
    prt::vector<ColliderTag> modelTags;
    prt::vector<Transform> modelTransforms;

    for (auto it = m_colliderUpdateSet.begin(); it != m_colliderUpdateSet.end(); it++) {
        ColliderTag tag =  m_entities.colliderTags[it->value()];
        switch (tag.type) {
            case COLLIDER_TYPE_MODEL:
                modelTags.push_back(tag);
                modelTransforms.push_back(m_entities.transforms[it->value()]);
                break;
            case COLLIDER_TYPE_ELLIPSOID:
                break;
            default:
                break;
        }
    }

    m_physicsSystem.updateModelColliders(modelTags.data(), modelTransforms.data(), modelTags.size());

    m_colliderUpdateSet = prt::hash_set<EntityID>();
}

void Scene::addModelCollider(EntityID id) {
    assert(hasModel(id));
    m_entities.colliderTags[id] = m_physicsSystem.addModelCollider(getModel(id), m_entities.transforms[id]);
}

void Scene::addEllipsoidCollider(EntityID id, glm::vec3 const & radii, glm::vec3 const & offset) {
    m_entities.colliderTags[id] = m_physicsSystem.addEllipsoidCollider(radii, offset);
}

void Scene::addPointLight(EntityID id, PointLight & pointLight) {
    pointLight.entityID = id;
    m_entities.lightTags[id] = m_lightingSystem.addPointLight(pointLight);
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
    float maxDist = 8.0f;
    float dist = maxDist;
    m_camera.getCameraCorners(corners[0], corners[1], corners[2], corners[3]);

    EntityID playerID = m_characterSystem.getPlayer();

    auto const & transform = m_entities.transforms[playerID];
    glm::vec3 offset = glm::vec3{0.0f, 2.0f, 0.0f};
    for (size_t i = 0; i < 4; ++i) {
        glm::vec3 dir = glm::normalize(corners[i] - (transform.position + offset));
        if (m_physicsSystem.raycast(transform.position + offset, dir, 
                                    maxDist, hit)) {
            dist = std::min(dist, glm::distance(transform.position + offset, hit));

        }
    }
    m_camera.setTargetDistance(dist);
    m_camera.setTarget(transform.position + offset);
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

    size_t colliderCount = 0;
    for (EntityID i = 0; i < m_entities.size(); ++i) {
        ColliderTag tag = m_entities.colliderTags[i];

        if (tag.type == COLLIDER_TYPE_ELLIPSOID) {
            EllipsoidCollider const & col = m_physicsSystem.getEllipsoidCollider(tag);
            glm::vec3 pos = m_entities.transforms[i].position + col.offset;
            glm::vec3 scale = col.radii;
            
            glm::mat4 tform = glm::translate(glm::mat4(1.0f), pos) * glm::scale(scale);

            m_renderData.colliderTransforms[colliderCount] = tform;
            ++colliderCount;
        }
    }
}

void Scene::initSky() {
    m_moon.billboard.color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_moon.billboard.size = {25.0f, 25.0f};
    m_moon.billboard.textureIndex = m_assetManager.getTextureManager().loadTexture("moon/moon_albedo.png");
    m_moon.distance = 200.0f;
}

void Scene::loadColliderModels() {
    char relative[512] = {0};
    io_util::getRelativePath(m_assetManager.getDirectory().c_str(), "colliders/ellipsoid.obj", relative);

    EntityID id = m_assetManager.getModelManager().loadModel(relative, false);
    m_colliderModelIDs.ellipsoid = id;
}

bool Scene::loadModel(EntityID entityID, char const * path, bool loadAnimation, bool isAbsolute) {
    bool alreadyLoaded;

    ModelID id;
    if (isAbsolute) {
        char relative[512] = {0};

        io_util::getRelativePath(m_assetManager.getDirectory().c_str(), path, relative);

        id = m_assetManager.getModelManager().loadModel(relative, loadAnimation, alreadyLoaded);
    } else {
        id = m_assetManager.getModelManager().loadModel(path, loadAnimation, alreadyLoaded);
    }

    if (id != -1) {
        m_entities.modelIDs[entityID] = id;

        if (m_entities.colliderTags[entityID].type == COLLIDER_TYPE_MODEL) {
            m_physicsSystem.removeCollider(m_entities.colliderTags[entityID]);
            m_entities.colliderTags[entityID] = m_physicsSystem.addModelCollider(m_assetManager.getModelManager().getModel(id), 
                                                                                 m_entities.transforms[entityID]);
            addToColliderUpdateSet(entityID);
        }

        if (!alreadyLoaded) {
            m_updateModels = true;
        }
    }

    return id != -1;
}
