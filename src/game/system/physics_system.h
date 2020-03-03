#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "src/graphics/geometry/model.h"

#include "src/container/vector.h"
#include "src/container/hash_map.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 position = {0.0f,0.0f,0.0f};
    glm::quat rotation = {1.0f,0.0f,0.0f,0.0f};
    glm::vec3 scale = {1.0f,1.0f,1.0f};

    glm::mat4 transformMatrix() {
        glm::mat4 scaleM = glm::scale(scale);
        glm::mat4 rotateM = glm::toMat4(rotation);
        glm::mat4 translateM = glm::translate(glm::mat4(1.0f), position);
        return translateM * rotateM * scaleM;
    }
};

class PhysicsSystem {
public:
    PhysicsSystem();

    void resolveEllipsoidsTriangles(const uint32_t* ellipsoidIDs,
                                    Transform* ellipsoidTransforms,
                                    glm::vec3* ellipsoidVelocities,
                                    bool* ellipsoidsAreGrounded,
                                    glm::vec3* ellipsoidGroundNormals,
                                    const size_t nEllipsoids,
                                    const uint32_t* triangleMeshIDs,
                                    const Transform* triangleTransforms,
                                    const size_t nTriangles,
                                    float deltaTime);

    void loadTriangleMeshColliders(const prt::vector<Model>& models,
                                   const prt::vector<uint32_t>& modelIDs);

    // uint32_t getTriangleMeshID(uint32_t modelID) {
    //     return modelIDToTriangleMeshIndex[modelID];
    // }

    uint32_t addEllipsoidCollider(glm::vec3 const& ellipsoid);
    uint32_t addTriangleMeshCollider(Model const& model);

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
    bool collideAndRespondEllipsoidTriangles(glm::vec3 const& ellipsoid, 
                                             Transform& ellipsoidTransform,
                                             glm::vec3& ellipsoidVel,
                                             bool& ellipsoidIsGrounded,
                                             glm::vec3& ellipsoidGroundNormal,
                                             prt::vector<glm::vec3> const& triangles,
                                             Transform const& triangleTransform,
                                             glm::vec3 const& trianglesVel,
                                             glm::vec3& intersectionPoint,
                                             float& intersectionTime);
        
private:
    // colliders
    struct TriangleMeshCollider {
        prt::vector<glm::vec3> triangles;
        float boundingSphere; 
    };
    // prt::hash_map<uint32_t, uint32_t> modelIDToTriangleMeshIndex;
    prt::vector<TriangleMeshCollider> triangleMeshColliders;

    prt::vector<glm::vec3> ellipsoids;

    bool collideEllipsoidTriangles(glm::vec3 const& ellipsoid, 
                                   glm::vec3 const& ellipsoidPos,
                                   glm::vec3 const& ellipsoidVel,
                                   bool& ellipsoidIsGrounded,
                                   glm::vec3& ellipsoidGroundNormal,
                                   prt::vector<glm::vec3> const& triangles,
                                   glm::vec3 const& trianglesPos,
                                   glm::vec3 const& trianglesVel,
                                   glm::vec3& intersectionPoint,
                                   float& intersectionTime);
                                   
    void respondEllipsoidTriangles(glm::vec3& ellipsoidPos,
                                   glm::vec3& ellipsoidVel,
                                   glm::vec3& intersectionPoint,
                                   const float intersectionTime);

    static constexpr float verySmallDistance = 0.005f;
};

#endif