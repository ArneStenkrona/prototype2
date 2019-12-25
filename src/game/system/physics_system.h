#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "src/container/vector.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PhysicsSystem {
public:

    /**
     * @param ellipsoid
     * @param ellipsoidPos
     * @param ellipsoidVel
     * @param triangles 
     * @param trianglesPos
     */
    bool collideEllipsoidTriangles(const glm::vec3& ellipsoid, 
                                   const glm::vec3& ellipsoidPos,
                                   const glm::vec3& ellipsoidVel,
                                   const prt::vector<glm::vec3>& triangles,
                                   const glm::vec3& trianglesPos,
                                   const glm::vec3& trianglesVel,
                                   glm::vec3& intersectionPoint,
                                   float& intersectionDistance);
private:
};

#endif