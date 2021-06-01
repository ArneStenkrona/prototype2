#ifndef PRT_COLLIDER_TAG_H
#define PRT_COLLIDER_TAG_H

#include <cstdint>

enum ColliderShape : uint8_t {
    COLLIDER_SHAPE_NONE,
    COLLIDER_SHAPE_MODEL,
    COLLIDER_SHAPE_MESH,
    COLLIDER_SHAPE_CAPSULE,
    TOTAL_NUM_COLLIDER_SHAPES
};

enum ColliderType : uint8_t {
    COLLIDER_TYPE_ERROR,
    COLLIDER_TYPE_TRIGGER,
    COLLIDER_TYPE_COLLIDE,
    TOTAL_NUM_COLLIDER_TYPES
};

typedef uint16_t ColliderIndex;

struct ColliderTag {
    ColliderIndex index;
    ColliderShape shape = ColliderShape::COLLIDER_SHAPE_NONE;
    ColliderType type = ColliderType::COLLIDER_TYPE_ERROR;
    friend bool operator== (ColliderTag const & c1, ColliderTag const & c2) {
        return (c1.index == c2.index &&
                c1.shape == c2.shape &&
                c1.type == c2.type);
    }
    friend bool operator!= (ColliderTag const & c1, ColliderTag const & c2)  {
        return !(c1 == c2);
    }
};

#endif