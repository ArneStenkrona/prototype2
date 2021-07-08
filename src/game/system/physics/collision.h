#ifndef ELLIPSOID_COLLIDER_H
#define ELLIPSOID_COLLIDER_H

#include "src/game/system/physics/colliders.h"
#include "src/game/system/physics/collider_tag.h"
#include "src/game/system/character/character.h"

#include <glm/glm.hpp>


struct CollisionResult {
    glm::vec3 intersectionPoint;
    glm::vec3 impulse;
    glm::vec3 collisionNormal;
    float     collisionDepth;
};

struct CollisionPackage {
    Scene * scene = nullptr;
    ColliderType type;
    Transform * transform = nullptr;;
    Character * character = nullptr;
};

void collideCapsuleMesh(CollisionPackage &      package,
                        CapsuleCollider const & capsule,
                        Polygon const *         polygons,
                        size_t                  nPolygons);

void collideCapsuleCapsule(CollisionPackage &      packageA,
                           CapsuleCollider const & capsuleA,
                           CollisionPackage &      packageB,
                           CapsuleCollider const & capsuleB);

void handleCollision(CollisionPackage & package,
                     CollisionResult const & result);

#endif
