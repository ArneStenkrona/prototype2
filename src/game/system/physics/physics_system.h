#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "src/game/component/component.h"
#include "src/game/system/physics/aabb.h"
#include "src/game/system/physics/aabb_tree.h"
#include "src/game/system/physics/colliders.h"
#include "src/game/system/physics/collision.h"
#include "src/graphics/geometry/model.h"
#include "src/system/assets/model_manager.h"
#include "src/game/system/character/character.h"

#include "src/container/vector.h"
#include "src/container/hash_map.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class PhysicsSystem {
public:
    PhysicsSystem();

    void updateCapsuleCollider(ColliderTag const & tag, 
                                 float height, 
                                 float radius,
                                 glm::vec3 const & offset);

    void updateModelColliders(ColliderTag const * tags,
                              Transform const * transforms,
                              size_t count);
    
    /**
     * Checks hit between ray and active colliders
     * 
     * @param origin ray origin
     * @param direction ray direction
     * @param maxDistance maximum distance ray may travel from origin
     * @param hit point of raycast hit, return by reference
     */
    bool raycast(glm::vec3 const& origin,
                 glm::vec3 const& direction,
                 float maxDistance,
                 glm::vec3 & hit);

    /**
     * Updates physics for character entities
     * 
     * @param deltaTime delta time
     * @param physics base pointer to character physics component
     * @param transforms base pointer to character transforms
     * @param n number of character entities
     */
    void updateCharacters(float deltaTime,
                          Scene & scene,
                          Transform * transforms);

    void updateTriggers(float deltaTime,
                        ColliderTag const * triggers,
                        Transform * transforms,
                        size_t n);

    ColliderTag addCapsuleCollider(float height,
                                   float radius,
                                   glm::vec3 const & offset);
                                   
    ColliderTag addModelCollider(Model const & model, Transform const & transform);

    void removeCollider(ColliderTag const & tag);

    CapsuleCollider & getCapsuleCollider(ColliderTag tag) { assert(tag.shape == COLLIDER_SHAPE_CAPSULE); return m_capsules[tag.index]; }

    float getGravity() const { return m_gravity; }
        
private:
    prt::vector<CapsuleCollider> m_capsules;

    // geometric data for model colliders
    struct Geometry {
        // raw geometric data
        prt::vector<glm::vec3> raw;
        // caches geometry after applying transforms
        prt::vector<glm::vec3> cache;
    };

    struct ModelColliderData {
        prt::vector<ModelCollider> models;
        prt::vector<MeshCollider> meshes;
        prt::vector<Geometry> geometries;

        prt::vector<unsigned int> freeList;
    } m_models;

    // dynamic aabb tree data
    struct TreeData {
        prt::vector<AABB> meshAABBs;
        prt::vector<int32_t> meshIndices;
        prt::vector<AABB> capsuleAABBs;
        prt::vector<int32_t> capsuleIndices;

        DynamicAABBTree tree;
    } m_aabbData;
    
    struct CollisionEvent {
        EntityID entityID;
        EntityID otherEntityID;
        CollisionResult result;
    };

    struct EventData {
        prt::vector<CollisionEvent> collisions;
        prt::vector<CollisionEvent> triggers;
        prt::hash_map<EntityID, prt::vector<unsigned int> > entityToEvents;
    } m_events;

    float m_gravity = 1.0f;

    void removeModelCollider(ColliderIndex colliderIndex);

    void collideCharacterWithWorld(Scene & scene,                                 
                                   Transform * transforms,
                                   uint32_t characterIndex,
                                   prt::hash_map<uint16_t, size_t> const & tagToCharacter);

    void collisionResponse(glm::vec3 const & intersectionPoint,
                           glm::vec3 const & collisionNormal,
                           float const intersectionTime,
                           CharacterPhysics * physics,
                           Transform * transforms,
                           uint32_t characterIndex);
};

#endif
