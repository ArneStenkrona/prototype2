#ifndef PRT_SCENE_H
#define PRT_SCENE_H

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "src/system/assets/model_manager.h"

class Scene {
public:
    Scene(ModelManager &modelManager);

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

    ModelManager &_modelManager;
};

#endif