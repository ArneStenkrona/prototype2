#ifndef PRT_COLLIDER_TAG_H
#define PRT_COLLIDER_TAG_H

enum ColliderType : uint16_t {
    COLLIDER_TYPE_NONE,
    COLLIDER_TYPE_MESH,
    COLLIDER_TYPE_ELLIPSOID,
    COLLIDER_TYPE_MODEL,
    TOTAL_NUM_COLLIDER_TYPES
};

struct ColliderTag {
    uint16_t index;
    ColliderType type = ColliderType::COLLIDER_TYPE_NONE;
    friend bool operator== (ColliderTag const & c1, ColliderTag const & c2) {
        return (c1.index == c2.index &&
                c1.type == c2.type);
    }
    friend bool operator!= (ColliderTag const & c1, ColliderTag const & c2)  {
        return !(c1 == c2);
    }
};

#endif