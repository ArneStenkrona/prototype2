#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"
#include "src/game/scene/scene_serialization.h"
#include "src/system/assets/asset_manager.h"
#include "src/graphics/camera/camera.h"
#include "src/graphics/lighting/light.h"
#include "src/system/input/input.h"
#include "src/game/system/animation/animation_system.h"
#include "src/game/system/physics/physics_system.h"
#include "src/game/system/character/character_system.h"
#include "src/game/system/character/character.h"
#include "src/graphics/vulkan/game_renderer.h"
#include "src/game/scene/entity.h"

#include "src/container/vector.h"
#include "src/container/hash_map.h"
#include "src/container/hash_set.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct RenderData {
    Texture const * textures;
    size_t nTextures;

    Model const * models;
    size_t nModels;

    prt::vector<glm::mat4> staticTransforms;
    prt::vector<EntityID>  staticEntityIDs;
    prt::vector<ModelID>   staticModelIDs;

    prt::vector<glm::mat4> animatedTransforms;
    prt::vector<EntityID>  animatedEntityIDs;
    prt::vector<ModelID>   animatedModelIDs;
    prt::vector<uint32_t>  boneOffsets;

    prt::vector<ModelID>   colliderModelIDs;
    prt::vector<glm::mat4> colliderTransforms;
};

class Scene {
public:
    Scene(GameRenderer & gameRenderer, 
          AssetManager & assetManager, 
          PhysicsSystem & physicsSystem,
          Input & input);

    void bindToRenderer();

    void update(float deltaTime);

    AnimationSystem & getAnimationSystem() { return m_animationSystem; }
    CharacterSystem & getCharacterSystem() { return m_characterSystem; }

    prt::vector<glm::mat4> const & sampleAnimation();

    void getSkybox(prt::array<Texture, 6>& cubeMap) const;

    void renderScene(Camera & camera);

    void addToColliderUpdateSet(EntityID id) { m_colliderUpdateSet.insert(id); }

    Transform & getTransform(EntityID id) { return m_entities.transforms[id]; }

    Input & getInput() { return m_input; }
    Camera & getCamera() { return m_camera; }

    Entities & getEntities() { return m_entities; }

    bool hasModel(EntityID id) const { return m_entities.modelIDs[id] != -1; }
    ModelID getModelID(EntityID id) const { return m_entities.modelIDs[id]; }
    Model const & getModel(EntityID id) { return m_assetManager.getModelManager().getModel(m_entities.modelIDs[id]); }

    bool hasCollider(EntityID id) const { return m_entities.colliderTags[id].shape != COLLIDER_SHAPE_NONE; }
    void addModelCollider(EntityID id);
    void addCapsuleCollider(EntityID id, float height, float radius, glm::vec3 const & offset);
    void updateCapsuleCollider(EntityID id, float height, float radius, glm::vec3 const & offset) { return m_physicsSystem.updateCapsuleCollider(m_entities.colliderTags[id], height, radius, offset); }
    CapsuleCollider & getCapsuleCollider(EntityID id) const { return m_physicsSystem.getCapsuleCollider(m_entities.colliderTags[id]); }

    bool isCharacter(EntityID id) { return m_entities.characterIDs[id] != -1; }
    CharacterType getCharacterType(EntityID id) { return m_characterSystem.getCharacter(id).attributeInfo.type; }

    bool loadModel(EntityID entityID, char const * path, bool loadAnimation, bool isAbsolute = true);

    char const * getAssetDirectory() const {  return m_assetManager.getDirectory().c_str(); }
    
    void addPointLight(EntityID id, PointLight & pointLight);
    PointLight & getPointLight(EntityID id) { return m_lightingSystem.getPointLight(m_entities.lightTags[id]); }
    
private:
    struct Lights {
        SkyLight sun;
    } m_lights;

    struct ColliderModelIDs {
        ModelID capsuleCap;
        ModelID capsuleBody;
    } m_colliderModelIDs;

    Entities m_entities;

    prt::hash_set<EntityID> m_colliderUpdateSet;
    bool m_updateModels = false;

    struct {
        Billboard billboard;
        glm::vec4 position;
        float distance;
    } m_moon;

    float time = 0.0f;

    GameRenderer & m_gameRenderer;
    AssetManager& m_assetManager;
    PhysicsSystem& m_physicsSystem;
    Input& m_input;
    Camera m_camera;
    AnimationSystem m_animationSystem;
    CharacterSystem m_characterSystem;
    LightingSystem m_lightingSystem;
    
    RenderData m_renderData;
    RenderResult m_renderResult;

    void bindRenderData();
    
    void initSky();
    void loadColliderModels();
    
    prt::vector<UBOPointLight> getPointLights();
    
    void updateSun(float time);
    void updatePhysics(float deltaTime);
    void updateColliders();
    void updateModels();
    void updateCamera(float deltaTime);
    void updateRenderData();

    friend class SceneSerialization;
    friend class Editor;
};

#endif