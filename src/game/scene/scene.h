#ifndef PRT_SCENE_H
#define PRT_SCENE_H

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
    struct StaticEntities {
        uint32_t modelIDs[MAXIMUM_STATIC_ENTITIES];
        glm::vec3 positions[MAXIMUM_STATIC_ENTITIES];
        glm::fquat rotations[MAXIMUM_STATIC_ENTITIES];
        glm::vec3 scales[MAXIMUM_STATIC_ENTITIES];
        //size_t num = 0;
    } _staticEntities;

    void resetTransforms();

    ModelManager &_modelManager;
};

#endif