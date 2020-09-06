#ifndef COLLIDERS_H
#define COLLIDERS_H

#include "src/game/component/component.h"
#include "src/graphics/geometry/model.h"
#include "src/game/system/physics/aabb.h"
#include "src/game/system/physics/bounding_box.h"

#include "src/container/vector.h"
#include "src/container/array.h"

#include <glm/glm.hpp>

enum ColliderType : uint16_t {
    MESH,
    ELLIPSOID,
    TOTAL_NUM_COLLIDER_TYPES
};

struct ColliderTag {
    uint16_t index;
    ColliderType type;
    friend bool operator== (ColliderTag const & c1, ColliderTag const & c2) {
        return (c1.index == c2.index &&
                c1.type == c2.type);
    }
    friend bool operator!= (ColliderTag const & c1, ColliderTag const & c2)  {
        return !(c1 == c2);
    }
};

struct MeshCollider {
    Transform transform;

    BoundingBox boundingBox;
    // AABB aabb;

    size_t startIndex;
    size_t numIndices;

    bool hasMoved;
};

struct ModelCollider {
    // transform applied to mesh colliders
    // Transform transform;
    // index of the first mesh collider
    size_t startIndex;
    // number of mesh colliders
    size_t numIndices;
    // has the model moved since last computation?
    // bool hasMoved;
};

#endif
