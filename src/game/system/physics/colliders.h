#ifndef COLLIDERS_H
#define COLLIDERS_H

#include "src/game/component/component.h"
#include "src/graphics/geometry/model.h"
#include "src/game/system/physics/aabb.h"
#include "src/game/system/physics/collider_tag.h"

#include "src/container/vector.h"
#include "src/container/array.h"

#include <glm/glm.hpp>

struct CapsuleCollider {
    float height;
    float radius;
    glm::vec3 offset;
    
    AABB getAABB(glm::mat4 const & transform) const {
        glm::vec4 a{offset, 1.0f};
        glm::vec4 b{offset + glm::vec3{0.0f, height, 0.0f}, 1.0f};

        glm::vec3 tA = transform * a;
        glm::vec3 tB = transform * b;
        
        return { glm::min(tA, tB) - glm::vec3{radius}, glm::max(tA, tB) + glm::vec3{radius} }; 
    }
};

struct Polygon {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 & operator[](size_t i) {  return *(&a + i); };
    glm::vec3 const & operator[](size_t i) const {  return *(&a + i); };
};

struct MeshCollider {
    Transform transform;

    unsigned int startIndex;
    unsigned int numIndices;

    unsigned int modelIndex;

    bool hasMoved;
};

struct ModelCollider {
    // index of the first mesh collider
    unsigned int startIndex;
    // // number of mesh colliders
    unsigned int numIndices;
};

#endif
