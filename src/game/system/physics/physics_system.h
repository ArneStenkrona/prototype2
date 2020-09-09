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

    uint16_t addEllipsoidCollider(glm::vec3 const & ellipsoid, int32_t characterIndex);
    void addModelColliders(uint32_t const * modelIDs, Transform const * transforms,
                           size_t count, uint32_t * ids);

    float getGravity() const { return m_gravity; }
        
private:
    prt::vector<glm::vec3> m_ellipsoids;

    ModelManager & m_modelManager;

    prt::vector<ModelCollider> m_modelColliders;
    prt::vector<MeshCollider> m_meshColliders;

    // caches geometry after applying transforms
    prt::vector<glm::vec3> m_geometryCache;
    prt::vector<glm::vec3> m_geometry;

    // collider meta data
    prt::vector<AABB> m_meshAABBs;
    prt::vector<int32_t> m_meshTreeIndices;
    prt::vector<AABB> m_ellipsoidAABBs;
    prt::vector<int32_t> m_ellipsoidTreeIndices;
    prt::vector<uint32_t> m_ellipsoidCharacterIndices;
    // aabb tree
    DynamicAABBTree m_aabbTree;

    float m_gravity = 1.0f;

    uint32_t addModelCollider(Model const & model, Transform const & transform);

    void collideCharacterwithWorld(CharacterPhysics * physics,
                                   Transform * transforms,
                                   size_t n,
                                   uint32_t characterIndex,
                                   bool & grounded);

    bool collideCharacterWithMeshes(glm::vec3 const & position, 
                                    glm::vec3 const & velocity, 
                                    glm::vec3 const & ellipsoidRadii,
                                    prt::vector<uint16_t> const & colliderIDs,
                                    glm::vec3 & intersectionPoint,
                                    float & intersectionTime,
                                    glm::vec3 & collisionNormal);

    bool collideCharacterWithCharacters(CharacterPhysics * physics,
                                        Transform * transforms,
                                        size_t n,
                                        uint32_t characterIndex,
                                        prt::vector<uint16_t> const & colliderIDs,
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
