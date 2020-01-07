#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "src/graphics/geometry/model.h"

#include "src/container/vector.h"
#include "src/container/hash_map.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PhysicsSystem {
public:
    PhysicsSystem();

    void loadTriangleMeshColliders(const prt::vector<Model>& models,
                                   const prt::vector<uint32_t>& modelIDs);

    uint32_t getTriangleMeshID(uint32_t modelID) {
        return modelIDToTriangleMeshIndex[modelID];
    }

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
     */
    bool collideAndRespondEllipsoidTriangles(const glm::vec3& ellipsoid, 
                                             glm::vec3& ellipsoidPos,
                                             glm::vec3& ellipsoidVel,
                                             const prt::vector<glm::vec3>& triangles,
                                             const glm::vec3& trianglesPos,
                                             const glm::vec3& trianglesVel,
                                             glm::vec3& intersectionPoint,
                                             float& intersectionTime);
        
private:
    // colliders
    struct TriangleMeshCollider {
        prt::vector<glm::vec3> triangles;
        float boundingSphere; 
    };
    prt::hash_map<uint32_t, uint32_t> modelIDToTriangleMeshIndex;
    prt::vector<TriangleMeshCollider> triangleMeshes;
    prt::vector< glm::vec3 > ellipsoids;

    bool collideEllipsoidTriangles(const glm::vec3& ellipsoid, 
                                   const glm::vec3& ellipsoidPos,
                                   const glm::vec3& ellipsoidVel,
                                   const prt::vector<glm::vec3>& triangles,
                                   const glm::vec3& trianglesPos,
                                   const glm::vec3& trianglesVel,
                                   glm::vec3& intersectionPoint,
                                   float& intersectionTime);
                                   
    void respondEllipsoidTriangles(glm::vec3& ellipsoidPos,
                                   glm::vec3& ellipsoidVel,
                                   glm::vec3& intersectionPoint,
                                   const float intersectionTime);

    static constexpr float verySmallDistance = 0.005f;
};

#endif