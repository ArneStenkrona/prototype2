#ifndef COLLIDERS_H
#define COLLIDERS_H

#include "src/game/component/component.h"
#include "src/graphics/geometry/model.h"
#include "src/game/system/physics/aabb.h"
#include "src/game/system/physics/bounding_box.h"
#include "src/game/system/physics/collider_tag.h"

#include "src/container/vector.h"
#include "src/container/array.h"

#include <glm/glm.hpp>

struct MeshCollider {
    Transform transform;

    BoundingBox boundingBox;
    // AABB aabb;
    
    unsigned int startIndex;
    unsigned int numIndices;

    unsigned int geometryIndex;

    bool hasMoved;
};

struct ModelCollider {
    // transform applied to mesh colliders
    // Transform transform;
    // index of the first mesh collider
    unsigned int startIndex;
    // // number of mesh colliders
    unsigned int numIndices;
    // has the model moved since last computation?
    // bool hasMoved;
    // geometry index
    unsigned int geometryIndex;
};

#endif
