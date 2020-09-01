#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

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
    Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem,
          Input& input, Camera& camera);

    void bindToRenderer(GameRenderer & gameRenderer);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices, bool animated) const;

    void update(float deltaTime);

    void sampleAnimation(prt::vector<glm::mat4> & bones);

    void getSkybox(prt::array<Texture, 6>& cubeMap) const;


    DirLight const & getSun() const { return m_lights.sun; }
    
private:
    struct Lights {
        DirLight sun;
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

    AssetManager& m_assetManager;
    PhysicsSystem& m_physicsSystem;
    Input& m_input;
    // Camera& m_camera;
    CharacterSystem m_characterSystem;

    // TODO: remove this
    void resetTransforms();
    uint32_t const * getModelIDs(size_t & nModelIDs, bool animated) const;

    void getNonAnimatedModels(Model const * & models, size_t & nModels, 
                              uint32_t const * & modelIDs, size_t & nModelIDs) const;
    void getAnimatedModels(Model const * & models, 
                           uint32_t const * & boneOffsets,
                           size_t & nModels, 
                           uint32_t const * & modelIDs, size_t & nModelIDs);

    void initColliders();

    void updatePhysics();
    void updateCamera();
};

#endif