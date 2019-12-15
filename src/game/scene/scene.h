#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/config/prototype2Config.h"

#include "src/system/assets/asset_manager.h"
#include "src/graphics/geometry/parametric_shapes.h"

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Scene {
public:
    Scene(AssetManager &assetManager);

    void getEntities(prt::vector<Model>& models, prt::vector<uint32_t>& modelIndices);

    void getTransformMatrixes(prt::vector<glm::mat4>& transformMatrices);
    
private:
    struct ModelEntities {
        uint32_t modelIDs[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 positions[MAXIMUM_MODEL_ENTITIES];
        glm::quat rotations[MAXIMUM_MODEL_ENTITIES];
        glm::vec3 scales[MAXIMUM_MODEL_ENTITIES];
        size_t numEntities = 0;
    } _modelEntities;

    AssetManager& _assetManager;
    
    void resetTransforms();
    void getModelIDs(prt::vector<uint32_t>& modelIDs);

};

#endif