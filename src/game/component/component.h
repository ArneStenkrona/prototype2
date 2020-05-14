#ifndef COMPONENT_H
#define COMPONENT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 position = {0.0f,0.0f,0.0f};
    glm::quat rotation = {1.0f,0.0f,0.0f,0.0f};
    glm::vec3 scale = {1.0f,1.0f,1.0f};

    glm::mat4 transformMatrix() const {
        glm::mat4 scaleM = glm::scale(scale);
        glm::mat4 rotateM = glm::toMat4(rotation);
        glm::mat4 translateM = glm::translate(glm::mat4(1.0f), position);
        return translateM * rotateM * scaleM;
    }
};

#endif