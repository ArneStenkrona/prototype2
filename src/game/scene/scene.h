#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

#include "src/system/assets/asset_manager.h"
#include "src/graphics/camera/camera.h"
#include "src/graphics/lighting/light.h"
#include "src/system/input/input.h"
#include "src/game/system/physics/physics_system.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Scene {
public:
    Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem,
          Input& input, Camera& camera);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices);

    void update(float deltaTime);

    void getModels(Model const * & models, size_t & nModels, 
                   prt::vector<uint32_t> & modelIDs, bool animated) const;

    void getSkybox(prt::array<Texture, 6>& cubeMap) const;

    DirLight const & getSun() const { return m_lights.sun; }
    
private:
    struct Lights {
        DirLight sun;
    } m_lights;
    
    template<size_t N>
    struct StaticEntities {
        enum { maxSize = N };
        size_t size = 0;

        uint32_t modelIDs[N];
        Transform transforms[N];
    };
    StaticEntities<10> m_staticEntities;

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
        uint32_t modelID;
        Transform transform;
        glm::vec3 velocity;
        glm::vec3 direction;
        float acceleration;
        float friction;
        glm::vec3 gravityVelocity;
        uint32_t ellipsoidColliderID;
        bool isGrounded;
        glm::vec3 groundNormal;
        bool jump;
    } m_playerEntity;

    AssetManager& m_assetManager;
    PhysicsSystem& m_physicsSystem;
    Input& m_input;
    Camera& m_camera;

    glm::vec3 m_gravityConstant;
    float m_gravity;
    
    void resetTransforms();
    void getModelIDs(prt::vector<uint32_t> & modelIDs, bool animated) const;

    void initColliders();

    void initPlayer();
    void updatePlayerInput();
    void updatePlayerPhysics(float deltaTime);
    void updatePhysics(float deltaTime);
    void updateCamera();
};

#endif