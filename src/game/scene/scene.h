#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

#include "src/system/assets/asset_manager.h"
#include "src/graphics/geometry/parametric_shapes.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>


class Scene {
public:
    Scene(AssetManager &assetManager);

    void getModelIDs(prt::vector<uint32_t>& modelIDs);
    void getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices);
    
private:
    struct ModelEntities {
        uint32_t modelIDs[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 positions[MAXIMUM_MODEL_ENTITIES];
        glm::fquat rotations[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 scales[MAXIMUM_MODEL_ENTITIES];
        //size_t num = 0;
    } _modelEntities;

    void resetTransforms();

    AssetManager &_assetManager;
};

#endif