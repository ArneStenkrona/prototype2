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
    PhysicsSystem(ModelManager & modelManager);

    void updateModelColliders(uint32_t const * colliderIDs,
                              Transform const * transforms,
                              size_t count);
    /**
     * Updates physics for character entities
     * 
     * @param physics base pointer to character physics component
     * @param transforms base pointer to character transforms
     * @param n number of character entities
     */
    void updateCharacterPhysics(CharacterPhysics * physics,
                                Transform * transforms,
                                size_t n);

    uint32_t addEllipsoidCollider(glm::vec3 const & ellipsoid);
    void addModelColliders(uint32_t const * modelIDs, Transform const * transforms,
                           size_t count, uint32_t * ids);

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

    float getGravity() const { return m_gravity; }
        
private:
    prt::vector<glm::vec3> ellipsoids;

    ModelManager & m_modelManager;

    prt::vector<ModelCollider> m_modelColliders;
    prt::vector<MeshCollider> m_meshColliders;

    // caches geometry after applying transforms
    prt::vector<glm::vec3> m_geometry_cache;
    prt::vector<glm::vec3> m_geometry;

    prt::vector<AABB> m_aabbs;
    prt::vector<int32_t> m_treeIndices;
    DynamicAABBTree m_aabbTree;

    float m_gravity = 1.0f;

    uint32_t addModelCollider(Model const & model, Transform const & transform);

    void collideCharacterwithWorld(glm::vec3 & ellipsoidPosition,
                                   glm::vec3 & ellipsoidVelocity,
                                   glm::vec3 const & ellipsoidRadii,
                                   glm::vec3 & groundNormal,
                                   bool & isGrounded);

    bool collideCharacterWithIDs(glm::vec3 & sourcePoint, 
                                 glm::vec3 & velocityVector, 
                                 glm::vec3 const & ellipsoidRadii,
                                 prt::vector<uint32_t> const & colliderIDs,
                                 glm::vec3 & intersectionPoint,
                                 float & intersectionTime,
                                 glm::vec3 & collisionNormal);
                   
    void respondCharacter(glm::vec3 & position,
                          glm::vec3 & velocity,
                          glm::vec3 & intersectionPoint,
                          float const intersectionTime);
};

#endif
