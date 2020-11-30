#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"
#include "src/game/scene/scene_serialization.h"
#include "src/system/assets/asset_manager.h"
#include "src/graphics/camera/camera.h"
#include "src/graphics/lighting/light.h"
#include "src/system/input/input.h"
#include "src/game/system/physics/physics_system.h"
#include "src/game/system/character/character.h"
#include "src/graphics/vulkan/game_renderer.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


class Scene {
public:
    Scene(GameRenderer & gameRenderer, 
          AssetManager & assetManager, 
          PhysicsSystem & physicsSystem,
          Input & input);

    void bindToRenderer(GameRenderer & gameRenderer);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices, bool animated) const;

    void update(float deltaTime);

    void sampleAnimation(prt::vector<glm::mat4> & bones);

    void getSkybox(prt::array<Texture, 6>& cubeMap) const;
    
private:
    struct Lights {
        prt::vector<PointLight> pointLights;
        prt::vector<BoxLight> boxLights;
        SkyLight sun;
    } m_lights;

    template<size_t N>
    struct StaticSolidEntities {
        enum { maxSize = N };
        size_t size = 0;
        uint32_t modelIDs[N];
        Transform transforms[N];
        uint32_t colliderIDs[N];
    };
    StaticSolidEntities<10> m_staticSolidEntities;

    struct {
        Billboard billboard;
        glm::vec4 position;
        float distance;
    } m_moon;

    GameRenderer & m_gameRenderer;
    AssetManager& m_assetManager;
    PhysicsSystem& m_physicsSystem;
    Input& m_input;
    Camera m_camera;
    CharacterSystem m_characterSystem;

    uint32_t const * getModelIDs(size_t & nModelIDs, bool animated) const;

    void getNonAnimatedModels(Model const * & models, size_t & nModels, 
                              uint32_t const * & modelIDs, size_t & nModelIDs) const;
    void getAnimatedModels(Model const * & models, size_t & nModels, 
                           uint32_t const * & modelIDs, size_t & nModelIDs);

    void initColliders();

    void initSky();
    
    prt::vector<PointLight> getPointLights();
    prt::vector<PackedBoxLight> getBoxLights();
    
    void updateSun(float time);
    void updatePhysics(float deltaTime);
    void updateCamera(float deltaTime);
    void renderScene();

    friend class SceneSerialization;
};

#endif