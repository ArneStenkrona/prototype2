#ifndef LIGHT_H
#define LIGHT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct PointLight {
    alignas(16) glm::vec3 pos;
    alignas(4)  float a; // quadtratic term
    alignas(16) glm::vec3 color;
    alignas(4)  float b; // linear term
    alignas(4)  float c; // constant term
};

struct DirLight {
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
};

struct SkyLight {
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    alignas(4)  float phase;
};

struct PackedBoxLight {
    alignas(16) glm::vec3 min;
    alignas(16) glm::vec3 max;
    alignas(16) glm::vec3 color;
    alignas(16) glm::mat4 invtransform;
};

struct BoxLight {
    alignas(16) glm::vec3 min;
    alignas(16) glm::vec3 max;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 position;
    alignas(16) glm::quat rotation;
    alignas(16) glm::vec3 scale;
    glm::mat4 inverseTransform() const {
        return glm::scale(1.0f / scale) * glm::transpose(glm::toMat4(rotation)) * glm::translate(glm::mat4(1.0f), -position);
    }
};

#endif