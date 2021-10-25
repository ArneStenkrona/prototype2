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
    EntityID other;
};

struct CollisionPackage {
    ColliderTag tag;
    EntityID entity;
    Transform * transform = nullptr;;
    Character * character = nullptr;
};

struct AggregateMeshCollider {
    struct TagOffset {
        ColliderTag tag;
        unsigned int offset;
    };

    EntityID entityID;

    prt::vector<Polygon> polygons;
    prt::vector<TagOffset> tagOffsets;
};

struct CollisionSetEntry {
    CollisionResult result;
};

bool operator==(const CollisionSetEntry& lhs, const CollisionSetEntry& rhs);
bool operator!=(const CollisionSetEntry& lhs, const CollisionSetEntry& rhs);

template<> struct std::hash<CollisionSetEntry> {
    std::size_t operator()(CollisionSetEntry const& entry) const noexcept {
        return std::hash<EntityID>{}(entry.result.other);
    }
};

class CollisionSystem {
public:
    CollisionSystem();

    void newFrame();

    prt::vector<CollisionResult> queryCollision(EntityID entityID);
    prt::vector<CollisionResult> queryCollisionEntry(EntityID entityID);
    prt::vector<CollisionResult> queryCollisionExit(EntityID entityID);

    prt::vector<CollisionResult> queryTrigger(EntityID entityID);
    prt::vector<CollisionResult> queryTriggerEntry(EntityID entityID);
    prt::vector<CollisionResult> queryTriggerExit(EntityID entityID);

    void collideCapsuleMesh(Scene &                 scene,
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
                         CollisionResult const & resultA,
                         CollisionResult const & resultB);
private:
    prt::hash_map<EntityID, prt::hash_set<CollisionSetEntry> > m_entityToCollisions;
    prt::hash_map<EntityID, prt::hash_set<CollisionSetEntry> > m_entityToPrevCollisions;

    prt::hash_map<EntityID, prt::hash_set<CollisionSetEntry> > m_entityToTriggers;
    prt::hash_map<EntityID, prt::hash_set<CollisionSetEntry> > m_entityToPrevTriggers;
};

#endif
