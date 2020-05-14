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
                              Transform const *transforms,
                              size_t count);

    void resolveEllipsoidsModels(uint32_t const * ellipsoidIDs,
                                 Transform* ellipsoidTransforms,
                                 glm::vec3* ellipsoidVelocities,
                                 bool* ellipsoidsAreGrounded,
                                 glm::vec3* ellipsoidGroundNormals,
                                 size_t const nEllipsoids,
                                 uint32_t const * colliderIDs,
                                 // Transform const * triangleTransforms,
                                 size_t const nColliderIDs,
                                 float deltaTime);

    // void loadTriangleMeshColliders(prt::vector<Model> const & models,
    //                                prt::vector<uint32_t> const & modelIDs);

    // uint32_t getTriangleMeshID(uint32_t modelID) {
    //     return modelIDToTriangleMeshIndex[modelID];
    // }

    uint32_t addEllipsoidCollider(glm::vec3 const & ellipsoid);
    void addModelColliders(uint32_t const * modelIDs, size_t count, uint32_t * ids);
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
     */
    bool collideAndRespondEllipsoidMesh(glm::vec3 const& ellipsoid, 
                                        Transform & ellipsoidTransform,
                                        glm::vec3 & ellipsoidVel,
                                        bool & ellipsoidIsGrounded,
                                        glm::vec3 & ellipsoidGroundNormal,
                                        // prt::vector<glm::vec3> const& triangles,
                                        Transform const & meshTransform,
                                        // glm::vec3 const& trianglesVel,
                                        MeshCollider const & meshCollider,
                                        glm::vec3& intersectionPoint,
                                        float& intersectionTime);
        
private:
    static constexpr float verySmallDistance = 0.005f;
    
    // colliders
    // struct TriangleMeshCollider {
    //     prt::vector<glm::vec3> triangles;
    //     float boundingSphere; 
    // };
    // prt::hash_map<uint32_t, uint32_t> modelIDToTriangleMeshIndex;
    // prt::vector<TriangleMeshCollider> triangleMeshColliders;

    prt::vector<glm::vec3> ellipsoids;

    ModelManager & m_modelManager;

    prt::vector<ModelCollider> m_modelColliders;
    prt::vector<MeshCollider> m_meshColliders;
    prt::vector<glm::vec3> m_geometry;

    DynamicAABBTree m_aabbTree;

    uint32_t addMeshCollider(Model const & model);

    bool collideEllipsoidMesh(glm::vec3 const& ellipsoid, 
                              glm::vec3 const& ellipsoidPos,
                              glm::vec3 const& ellipsoidVel,
                              bool& ellipsoidIsGrounded,
                              glm::vec3& ellipsoidGroundNormal,
                              prt::vector<glm::vec3> const& triangles,
                              glm::vec3 const& trianglesPos,
                              glm::vec3 const& trianglesVel,
                              glm::vec3& intersectionPoint,
                              float& intersectionTime);
                                   
    void respondEllipsoidMesh(glm::vec3& ellipsoidPos,
                              glm::vec3& ellipsoidVel,
                              glm::vec3& intersectionPoint,
                              const float intersectionTime);
};

#endif
