#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "src/game/component/component.h"
#include "src/game/system/physics/aabb.h"
#include "src/game/system/physics/aabb_tree.h"
#include "src/game/system/physics/colliders.h"
#include "src/graphics/geometry/model.h"
#include "src/system/assets/model_manager.h"

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
    void updateCharacterPhysics(float deltaTime,
                                CharacterPhysics * physics,
                                Transform * transforms,
                                size_t n);

    ColliderTag addEllipsoidCollider(glm::vec3 const & ellipsoid);
    ColliderTag addModelCollider(Model const & model, Transform const & transform);

    void removeCollider(ColliderTag const & tag);

    glm::vec3 getEllipsoidCollider(ColliderTag tag) const { assert(tag.type == COLLIDER_TYPE_ELLIPSOID); return m_ellipsoids[tag.index]; }

    float getGravity() const { return m_gravity; }
        
private:
    prt::vector<glm::vec3> m_ellipsoids;

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
        prt::vector<AABB> ellipsoidAABBs;
        prt::vector<int32_t> ellipsoidIndices;

        DynamicAABBTree tree;
    } m_aabbData;
    // aabb tree

    float m_gravity = 1.0f;

    void removeModelCollider(ColliderIndex index);

    void collideCharacterwithWorld(CharacterPhysics * physics,
                                   Transform * transforms,
                                   size_t n,
                                   uint32_t characterIndex,
                                   prt::hash_map<uint16_t, size_t> const & tagToCharacter,
                                   bool & grounded);

    bool collideCharacterWithMeshes(glm::vec3 const & position, 
                                    glm::vec3 const & velocity, 
                                    glm::vec3 const & ellipsoidRadii,
                                    prt::vector<ColliderIndex> const & colliderIndices,
                                    glm::vec3 & intersectionPoint,
                                    float & intersectionTime,
                                    glm::vec3 & collisionNormal);

    bool collideCharacterWithCharacters(CharacterPhysics * physics,
                                        Transform * transforms,
                                        size_t n,
                                        uint32_t characterIndex,
                                        prt::vector<ColliderIndex> const & colliderIndices,
                                        prt::hash_map<ColliderIndex, size_t> const & tagToCharacter,
                                        glm::vec3 & intersectionPoint,
                                        float & intersectionTime,
                                        glm::vec3 & collisionNormal,
                                        uint32_t & otherCharacterIndex);
                   
    bool collideEllipsoids(glm::vec3 const & ellipsoid0,
                           glm::vec3 const & sourcePoint0, 
                           glm::vec3 const & velocity0, 
                           glm::vec3 const & ellipsoid1, 
                           glm::vec3 const & sourcePoint1, 
                           glm::vec3 const & velocity1,
                           float & intersectionTime, 
                           glm::vec3 & intersectionPoint,
                           glm::vec3 & collisionNormal);

    bool computeContactEllipsoids(glm::mat3 const & D, 
                                  glm::vec3 const & K, 
                                  glm::vec3 const & W, 
                                  float & intersectionTime, 
                                  glm::vec3 & zContact);

    float computeClosestPointEllipsoids(glm::mat3 const & D, 
                                        glm::vec3 const & K, 
                                        glm::vec3 & closestPoint);

    void collisionResponse(bool collision,
                           glm::vec3 const & intersectionPoint,
                           glm::vec3 const & collisionNormal,
                           float const intersectionTime,
                           CharacterPhysics * physics,
                           Transform * transforms,
                           uint32_t characterIndex,
                           uint32_t otherCharacterIndex);
};

#endif
