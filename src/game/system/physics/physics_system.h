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

    void collisionDetection(glm::vec3 & ellipsoidPosition,
                            glm::vec3 & ellipsoidVelocity,
                            glm::vec3 const & ellipsoidRadii);

    void collideWithWorld(glm::vec3 & sourcePoint, 
                          glm::vec3 & velocityVector, 
                          glm::vec3 ellipsoidRadii,
                          prt::vector<uint32_t> const & colliderIDs);

    void resolveEllipsoidsModels(uint32_t const * ellipsoidIDs,
                                 Transform* ellipsoidTransforms,
                                 glm::vec3* ellipsoidVelocities,
                                 bool* ellipsoidsAreGrounded,
                                 glm::vec3* ellipsoidGroundNormals,
                                 size_t const nEllipsoids,
                                //  uint32_t const * colliderIDs,
                                 // Transform const * triangleTransforms,
                                //  size_t const nColliderIDs,
                                 float deltaTime);

    uint32_t addEllipsoidCollider(glm::vec3 const & ellipsoid);
    void addModelColliders(uint32_t const * modelIDs, Transform const * transforms,
                           size_t count, uint32_t * ids);
    /**
     * Checks for collision between an ellipsoid and a triangle mesh.
     * 
     * @param ellipsoid axes of the ellipsoid collider
     * @param ellipsoidPos position of the ellipsoid collider
     * @param ellipsoidVel velocity of the ellipsoid collider
     * @param triangles triangles of the collision mesh as a position list
     * @param trianglesPos position of the collision mesh
     * @param trianglesVel velocity of the triangle mesh
     * 
     * @param intersectionPoint intersection point in ellipsoid space, 
     *                          returned by reference
     * @param intersectionDistance intersection time,
     *                             returned by reference
     * @param collisionNormals list of collision normals
     * @return true if collision, false otherwise
     */
    bool collideAndRespondEllipsoidMesh(glm::vec3 const& ellipsoid, 
                                        Transform & ellipsoidTransform,
                                        glm::vec3 & ellipsoidVel,
                                        bool & ellipsoidIsGrounded,
                                        glm::vec3 & ellipsoidGroundNormal,
                                        // MeshCollider const & meshCollider,
                                        prt::vector<uint32_t> const & colliderIDs,
                                        glm::vec3& intersectionPoint,
                                        float& intersectionTime);

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
    static constexpr float verySmallDistance = 0.005f;

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

    bool collideEllipsoidMesh(glm::vec3 const & ellipsoid, 
                              glm::vec3 const & ellipsoidPos,
                              glm::vec3 const & ellipsoidVel,
                              bool& ellipsoidIsGrounded,
                              glm::vec3& ellipsoidGroundNormal,
                              prt::vector<glm::vec3> const& triangles,
                              glm::vec3 const & trianglesPos,
                              glm::vec3 const & trianglesVel,
                              glm::vec3 & intersectionPoint,
                              float & intersectionTime);

    // consider moving the below functions to a utility namespace
    glm::vec3 closestPointOnTrianglePerimeter(glm::vec3 const & a,
                                              glm::vec3 const & b,
                                              glm::vec3 const & c,
                                              glm::vec3 const & p);

    glm::vec3 closestPointOnLine(glm::vec3 const & a,
                                 glm::vec3 const & b,
                                 glm::vec3 const & p);
                                   
    void respondEllipsoidMesh(glm::vec3 & ellipsoidPos,
                              glm::vec3 & ellipsoidVel,
                              glm::vec3 & intersectionPoint,
                              float const intersectionTime);

    float intersectRayPlane(glm::vec3 const & planeOrigin, 
                            glm::vec3 const & planeNormal,
                            glm::vec3 const & rayOrigin,
                            glm::vec3 const & rayVector);

    float intersectSphere(glm::vec3 const & rO, 
                          glm::vec3 const & rV, 
                          glm::vec3 const & sO, 
                          float sR);

    bool intersectLineSegmentTriangle(glm::vec3 const & origin, 
                                      glm::vec3 const & end, 
                                      glm::vec3 const & a,
                                      glm::vec3 const & b,
                                      glm::vec3 const & c,
                                      float &t);

    glm::vec3 barycentric(glm::vec3 const & p, 
                          glm::vec3 const & a, 
                          glm::vec3 const & b, 
                          glm::vec3 const & c);
};

#endif
