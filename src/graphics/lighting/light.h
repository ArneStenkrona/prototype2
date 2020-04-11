#ifndef LIGHT_H
#define LIGHT_H

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

#endif