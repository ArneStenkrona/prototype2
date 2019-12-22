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

    void initPlayer();

    void getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices);

    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices);

    void updatePlayer(float deltaTime);
    
private:
    struct {
        uint32_t modelIDs[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 positions[MAXIMUM_MODEL_ENTITIES];
        glm::quat rotations[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 scales[MAXIMUM_MODEL_ENTITIES];
        size_t numEntities = 0;
    } _modelEntities;

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

};

#endif