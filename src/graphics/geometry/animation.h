#ifndef PRT_ANIMATION_H
#define PRT_ANIMATION_H

#include "src/container/vector.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Vertex;

struct VertexWeight {
    uint32_t vertexIndex;
    float weight;
};

struct Bone {
    uint32_t id;
    uint32_t vertexWeightIndex;
    uint32_t numVertexWeights;
    glm::mat4 offsetMatrix;
};

struct Mesh {
    uint32_t boneIndex;
    uint32_t numBones;

    uint32_t vertexIndex;
    uint32_t numVertices;
};

struct Model {
    prt::vector<Mesh> meshes;
    prt::vector<Bone> bones;
    prt::vector<VertexWeight> vertexWeights;
    prt::vector<Vertex> vertices;
};


struct Node {
    // transformation relative to parent node
    uint32_t id;
    glm::mat4 transformation;
    Node *parent; // this should probably be a handle instead of a pointer
    uint32_t numCgildren;
    Node **children;
};

struct VectorKey {
    float time;
    glm::vec3 value;
};

struct QuaternionKey {
    float time;
    glm::quat value;
};

struct NodeAnimation {
    uint32_t id;
    prt::vector<VectorKey> positionKeys;
    prt::vector<QuaternionKey> rotationKeys;
    prt::vector<VectorKey> scalingKeys;
};

struct Animation {
    float duration;
    float ticksPerSecond;
    prt::vector<NodeAnimation> channels;

};

#endif