#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "src/game/system/physics/colliders.h"
#include "src/game/system/physics/collider_tag.h"
#include "src/game/system/character/character.h"

#include <glm/glm.hpp>

struct CollisionResult {
    glm::vec3 intersectionPoint;
    glm::vec3 impulse;
    glm::vec3 collisionNormal;
    float     collisionDepth;
    ColliderTag tagA;
    ColliderTag tagB;
};

struct CollisionPackage {
    ColliderTag tag;
    Transform * transform = nullptr;;
    Character * character = nullptr;
};

struct AggregateMeshCollider {
    struct TagOffset {
        ColliderTag tag;
        unsigned int offset;
    };

    prt::vector<Polygon> polygons;
    prt::vector<TagOffset> tagOffsets;
};

class CollisionSystem {
public:
    CollisionSystem();

    void newFrame();

    void collideCapsuleMesh(Scene & scene,
                            CollisionPackage &      package,
                            CapsuleCollider const & capsule,
                            AggregateMeshCollider const & aggregateMeshCollider);

    void collideCapsuleCapsule(Scene & scene,
                               CollisionPackage &      packageA,
                               CapsuleCollider const & capsuleA,
                               CollisionPackage &      packageB,
                               CapsuleCollider const & capsuleB);

    void handleCollision(Scene & scene,
                         CollisionPackage & package,
                         CollisionResult const & result);
private:
    prt::vector<CollisionResult> m_collisions;
};

#endif
