#ifndef LIGHT_H
#define LIGHT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct UBOPointLight {
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
    alignas(16) glm::vec3 nightColor;
    alignas(16) glm::vec3 dayColor;
    alignas(16) glm::vec3 sunEdgeColor;
    alignas(16) glm::vec3 sunsetriseColor;
    alignas(16) glm::vec3 sunColor;
    alignas(4) float distToNoon;
};

#endif