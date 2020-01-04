#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

#include "src/system/assets/asset_manager.h"
#include "src/graphics/camera/camera.h"
#include "src/system/input/input.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Scene {
public:
    Scene(AssetManager &assetManager, Input& input, Camera& camera);


    void getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices);
    void getSkybox(prt::array<Texture, 6>& cubeMap);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices);

    void update(float deltaTime);

private:
    struct {
        uint32_t modelIDs[MAXIMUM_MODEL_ENTITIES];
        //uint32_t modelIDs[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 positions[MAXIMUM_MODEL_ENTITIES];
        glm::quat rotations[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 scales[MAXIMUM_MODEL_ENTITIES];
        size_t numEntities = 0;
    } _entities;

    enum RESERVED_ENTITY_IDS {
        PLAYER_ID,
        TOTAL_RESERVED_ENTIIY_IDS
    };

    struct {
        uint32_t entityID;
        float acceleration;
        float friction;
        glm::vec3 velocity;
    } _player;

    AssetManager& _assetManager;
    Input& _input;
    Camera& _camera;
    
    void resetTransforms();
    void getModelIDs(prt::vector<uint32_t>& modelIDs);

    void initPlayer();
    void updatePlayer(float deltaTime);
};

#endif