#ifndef PARAMETRIC_SHAPES_H
#define PARAMETRIC_SHAPES_H

#include "src/container/vector.h"

#include <cstdint>

struct Vertex;
namespace parametric_shapes {

struct Quad {
    float width = 1.0f;
    float height = 1.0f;
    uint32_t resW = 0;
    uint32_t resH = 0;
};

struct Cuboid {
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
    //uint32_t resW = 0;
    //uint32_t resH = 0;
    //uint32_t resD = 0;
};

struct Sphere {
    float radius = 1.0f;
    uint32_t res = 0;
};

struct Cylinder {
    float radius = 1.0f;
    float height = 1.0f;
    uint32_t res = 0;
};

struct Capsule {
    float radius = 1.0f;
    float height = 1.0f;
    uint32_t resR = 0;
    uint32_t resH = 0;
};

void createQuad(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Quad quad);

void createCuboid(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cuboid cuboid);

void createSphere(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Sphere sphere);

void createCylinder(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cylinder cylinder);

void createCapsule(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Capsule capsule);
};

#endif