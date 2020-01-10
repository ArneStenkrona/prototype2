#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

#include "src/system/assets/asset_manager.h"
#include "src/graphics/camera/camera.h"
#include "src/system/input/input.h"
#include "src/game/system/physics_system.h"
#include "src/graphics/vulkan/vulkan_application.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Scene {
public:
    Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem,
          Input& input, Camera& camera);

    void load(VulkanApplication& vulkanApplication);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices);

    void update(float deltaTime);

private:
    template<size_t N>
    struct StaticEntities {
        enum { maxSize = N };
        size_t size = 0;

        uint32_t modelIDs[N];
        Transform transforms[N];
    };
    StaticEntities<10> _staticEntities;

    template<size_t N>
    struct StaticSolidEntities {
        enum { maxSize = N };
        size_t size = 0;

        uint32_t modelIDs[N];
        Transform transforms[N];
        uint32_t triangleMeshColliderIDs[N];
    };
    StaticSolidEntities<10> _staticSolidEntities;

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
    } _playerEntity;

    AssetManager& _assetManager;
    PhysicsSystem& _physicsSystem;
    Input& _input;
    Camera& _camera;

    glm::vec3 _gravityConstant;
    float _gravity;
    bool _applyGravity = false;
    
    void resetTransforms();
    void getModelIDs(prt::vector<uint32_t>& modelIDs);

    void resolveColliderIDs();

    void getSkybox(prt::array<Texture, 6>& cubeMap);

    void initPlayer();
    void updatePlayerInput();
    void updatePlayerPhysics(float deltaTime);
    void updatePhysics(float deltaTime);
    void updateCamera();
};

#endif