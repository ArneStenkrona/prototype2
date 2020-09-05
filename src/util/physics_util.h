#ifndef PHYSICS_UTIL_H
#define PHYSICS_UTIL_H

#include "glm/glm.hpp"

namespace physics_util {
    /**
     * Computes the closest point on a triangle (a,b,c) 
     * to point p
     * @param a triangle vertex
     * @param b triangle vertex
     * @param c triangle vertex
     * @param p point
     * @return closest point
     */
    glm::vec3 closestPointOnTrianglePerimeter(glm::vec3 const & a,
                                              glm::vec3 const & b,
                                              glm::vec3 const & c,
                                              glm::vec3 const & p);

    /**
     * Computes the closest point on line that
     * intersects a and b, a != b, to point p
     * @param a point on line
     * @param b point on line
     * @param p point
     * @return closest point
     */
    glm::vec3 closestPointOnLine(glm::vec3 const & a,
                                 glm::vec3 const & b,
                                 glm::vec3 const & p);

    /**
     * Computes intersection of ray and plane
     * @param planeOrigin point on plane
     * @param planeNormal normalized normal of plane
     * @param rayOrigin origin of ray
     * @param rayVector normalized direction of ray
     * 
     * @param t resulting t such that (ray origin) + t * (ray direction)
     *          gives the intersection, if possible
     * @return true if intersection,
     *         false otherwise 
     */
    bool intersectRayPlane(glm::vec3 const & planeOrigin, 
                           glm::vec3 const & planeNormal,
                           glm::vec3 const & rayOrigin,
                           glm::vec3 const & rayVector,
                           float & t);

    /**
     * Computes intersection of ray and sphere
     * @param rayOrigin origin of ray
     * @param rayVector normalized direction of ray
     * @param sphereOrigin origin of sphere
     * 
     * @param t resulting t such that (ray origin) + t * (ray direction)
     *          gives the intersection, if possible
     * @return true if intersection,
     *         false otherwise 
     */
    bool intersectRaySphere(glm::vec3 const & rayOrigin, 
                            glm::vec3 const & rayDirection, 
                            glm::vec3 const & sphereOrigin, 
                            float sphereRadius,
                            float & t);

    /**
     * Computes intersection of line segment and triangle
     * @param origin origin of line segment
     * @param end end of line segment
     * @param a triangle vertex
     * @param b triangle vertex
     * @param c triangle vertex
     * 
     * @param t resulting t in [0,1] such that (1 - t) * origin + t * end
     *          gives the intersection, if possible
     * @return true if intersection,
     *         false otherwise 
     */
    bool intersectLineSegmentTriangle(glm::vec3 const & origin, 
                                      glm::vec3 const & end, 
                                      glm::vec3 const & a,
                                      glm::vec3 const & b,
                                      glm::vec3 const & c,
                                      float & t);
                                      
    /**
     * Checkes wether the orthogonal projection of
     * point onto triangle is contained within
     * the triangle
     * @param point point
     * @param pa triangle vertex
     * @param pb triangle vertex
     * @param pc triangle vertex
     * @return true if in triangle
     *         false otherwise
     */
    bool checkPointInTriangle(glm::vec3 const & point,
                              glm::vec3 const & pa, 
                              glm::vec3 const & pb, 
                              glm::vec3 const & pc);
};

#endif
