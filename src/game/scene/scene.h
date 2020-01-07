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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 position = {0.0f,0.0f,0.0f};
    glm::quat rotation = {1.0f,0.0f,0.0f,0.0f};
    glm::vec3 scale = {1.0f,1.0f,1.0f};

    glm::mat4 transformMatrix() {
        glm::mat4 scaleM = glm::scale(scale);
        glm::mat4 rotateM = glm::toMat4(rotation);
        glm::mat4 translateM = glm::translate(glm::mat4(1.0f), position);
        return translateM * rotateM * scaleM;
    }
};

class Scene {
public:
    Scene(AssetManager &assetManager, PhysicsSystem& physicsSystem,
          Input& input, Camera& camera);

    void load(VulkanApplication& vulkanApplication);


    // void getEntities(prt::vector<Model>& models, 
    //                  prt::vector<uint32_t>& modelIndices,
    //                  prt::vector<Model>& colliderModels,
    //                  prt::vector<uint32_t>& colliderIDs);

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
        float acceleration;
        float friction;
        uint32_t ellipsoidColliderID;
    } _playerEntity;

    AssetManager& _assetManager;
    PhysicsSystem& _physicsSystem;
    Input& _input;
    Camera& _camera;
    
    void resetTransforms();
    void getModelIDs(prt::vector<uint32_t>& modelIDs);

    void resolveColliderIDs();

    void getSkybox(prt::array<Texture, 6>& cubeMap);

    void initPlayer();
    void updatePlayer(float deltaTime);
};

#endif