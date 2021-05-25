#ifndef ELLIPSOID_COLLIDER_H
#define ELLIPSOID_COLLIDER_H

#include "src/game/system/physics/colliders.h"
#include "src/game/system/character/character.h"

#include <glm/glm.hpp>


struct CollisionResult {
    // bool collided;
    glm::vec3 intersectionPoint;
    glm::vec3 impulse;
    glm::vec3 collisionNormal;
    float     collisionDepth;
};

enum CollisionType {
    COLLISION_TYPE_ERROR,
    COLLISION_TYPE_TRIGGER,
    COLLISION_TYPE_RESPOND,
    TOTAL_NUM_COLLISION_TYPES
};

struct CollisionPackage {
    CollisionType type;
    Transform * transform;
    CharacterPhysics * physics;
};

void collideCapsuleMesh(CollisionPackage &      package,
                        CapsuleCollider const & capsule,
                        Polygon const *         polygons,
                        size_t                  nPolygons);

void collideCapsuleCapsule(CollisionPackage &      packageA,
                           CapsuleCollider const & capsuleA,
                           CollisionPackage &      packageB,
                           CapsuleCollider const & capsuleB);

bool collideSphereMesh(glm::vec3 const & sourcePoint, 
                       glm::vec3 const & relativeVelocity, 
                       Polygon   const * polygons,
                       size_t            nPolygons,
                       glm::vec3 &       intersectionPoint,
                       float     &       intersectionTime,
                       glm::vec3 &       collisionNormal);

bool collideEllipsoidEllipsoid(glm::vec3 const & ellipsoid0,
                               glm::vec3 const & sourcePoint0, 
                               glm::vec3 const & velocity0, 
                               glm::vec3 const & ellipsoid1, 
                               glm::vec3 const & sourcePoint1, 
                               glm::vec3 const & velocity1,
                               float &           intersectionTime, 
                               glm::vec3 &       intersectionPoint,
                               glm::vec3 &       collisionNormal);

bool computeContactEllipsoids(glm::mat3 const &   D, 
                                glm::vec3 const & K, 
                                glm::vec3 const & W, 
                                float &           intersectionTime, 
                                glm::vec3 &       zContact);

float computeClosestPointEllipsoids(glm::mat3 const & D, 
                                    glm::vec3 const & K, 
                                    glm::vec3 &       closestPoint);

void handleCollision(CollisionPackage & package,
                     CollisionResult const & result);

#endif
