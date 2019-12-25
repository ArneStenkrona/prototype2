#include "physics_system.h"

#include <glm/gtx/norm.hpp>

bool checkPointInTriangle(const glm::vec3& point,
                          const glm::vec3& pa, const glm::vec3& pb, const glm::vec3& pc)
{
    glm::vec3 ba = pb - pa;
    glm::vec3 cb = pc - pb;
    glm::vec3 ac = pa - pc;
    glm::vec3 n = glm::cross(ac, ba);

    glm::vec3 px = point - pa;
    glm::vec3 nx = glm::cross(ba, px);
    if (glm::dot(nx, n) < 0.0f) return false;

    px = point - pb;
    nx = glm::cross(cb, px);
    if (glm::dot(nx, n) < 0.0f) return false;

    px = point - pc;
    nx = glm::cross(ac, px);
    if (glm::dot(nx, n) < 0.0f) return false;

    return true;
}

bool getLowestRoot(float a, float b, float c, float maxR,
                float& root) {
    // Check if a solution exists
    float determinant = b*b - 4.0f*a*c;
    // If determinant is negative it means no solutions.
    if (determinant < 0.0f) return false;
    // calculate the two roots: (if determinant == 0 then
    // x1==x2 but let’s disregard that slight optimization)
    float sqrtD = sqrt(determinant);
    float r1 = (-b - sqrtD) / (2*a);
    float r2 = (-b + sqrtD) / (2*a);
    // Sort so x1 <= x2
    if (r1 > r2) {
        float temp = r2;
        r2 = r1;
        r1 = temp;
    }
    // Get lowest root:
    if (r1 > 0 && r1 < maxR) {
        root = r1;
        return true;
    }
    // It is possible that we want x2 - this can happen
    // if x1 < 0
    if (r2 > 0 && r2 < maxR) {
    root = r2;
        return true;
    }
    // No (valid) solutions
    return false;
}
/**
 * based on "Improved Collision detection and Response" by
 * Kasper Fauerby.
 * Link: http://www.peroxide.dk/papers/collision/collision.pdf
 */
bool PhysicsSystem::collideEllipsoidTriangles(const glm::vec3& ellipsoid, 
                                              const glm::vec3& ellipsoidPos,
                                              const glm::vec3& ellipsoidVel,
                                              const prt::vector<glm::vec3>& triangles,
                                              const glm::vec3& trianglesPos,
                                              const glm::vec3& trianglesVel,
                                              glm::vec3& intersectionPoint,
                                              float& intersectionDistance) {
    glm::vec3 pos = ellipsoidPos - trianglesPos;
    glm::vec3 vel = ellipsoidVel - trianglesVel;                                                 
    prt::vector<glm::vec3> tris = triangles;
    // convert to eSpace
    // change of basis matrix, though simplified as a vector
    glm::vec3 cbm = { 1.0f / ellipsoid.x, 1.0f / ellipsoid.y, 1.0f / ellipsoid.z };
    for (size_t i = 0; i < tris.size(); i++) {
        tris[i] = { cbm.x * tris[i].x, cbm.y * tris[i].y, cbm.z * tris[i].z };
    }
    pos = { cbm.x * pos.x, cbm.y * pos.y, cbm.z * pos.z };
    vel = { cbm.x * vel.x, cbm.y * vel.y, cbm.z * vel.z };

    // results of the collision test
    intersectionDistance = std::numeric_limits<float>::infinity();

    for (size_t i = 0; i < tris.size(); i+=3) {
        // find the triangle plane
        const glm::vec3& p1 = tris[i];
        const glm::vec3& p2 = tris[i+1];
        const glm::vec3& p3 = tris[i+2];
        // plane normal
        glm::vec3 n = glm::cross((p2-p1),(p3-p1));
        n = glm::normalize(n);

        // skip triangle if relative velocity is not towards the triangle
        // if (glm::dot(glm::normalize(vel), n) > 0.0f) {
        //     continue;
        // }

        // plane constant
        float c = -glm::dot(n, p1);

        float t0;
        float t1;
        float ndotv = glm::dot(n, vel);
        float sigDist = glm::dot(pos, n) + c;
        bool embeddedInPlane = false;
        // check distance to plane and set t0,t1
        if (ndotv != 0.0f) {
            t0 = (1.0f - sigDist) / ndotv;
            t1 = (-1.0f - sigDist) / ndotv;
            if ( t1 < 0.0f || t0 > 1.0f) {
                // collision occurs outside velocity range
                continue;
            }
            t0 = glm::clamp(t0, 0.0f, 1.0f);
            t1 = glm::clamp(t1, 0.0f, 1.0f);
        } else if (std::abs(sigDist) < 1.0f) {
            // sphere is embedded in plane
            t0 = 0.0f;
            t1 = 1.0f;
            embeddedInPlane = true;
        } else {
            // no collision
            continue;
        } 
        
        if (!embeddedInPlane) {
            // calculate plane intersection point;
            glm::vec3 pip = pos - n + t0 * vel;
            // check if we are colliding inside the triangle
            if (checkPointInTriangle(pip, p1, p2, p3)) {
                float intDist = t0 * glm::length(vel);
                if (intDist < intersectionDistance) {
                    intersectionDistance = intDist;
                    intersectionPoint = pip;
                }
                // collision inside triangle
                continue;
            }
        }
        /*
        // sweep against vertices and edges
        // vertex;
        for (size_t j = 0; j < 3; j++) {
            float av = glm::dot(vel, vel);
            float bv = 2.0f * glm::dot(vel, pos - tris[i + j]);
            float cv = glm::length2(tris[i + j] - pos) - 1.0f;
            float rv;
            if (getLowestRoot(av, bv, cv, 10000.0f, rv)) {
                float intDist = rv * glm::length(vel);
                if (intDist < intersectionDistance) {
                    intersectionDistance = intDist;
                    intersectionPoint = tris[i + j];
                }
            }
        }
        // edges
        glm::vec3 edges[3] = { p2 - p1, p3 - p2, p1 - p3 };
        glm::vec3 btv[3] = { p1 - pos, p2 - pos, p3 - pos }; // base to vertex
        for (size_t j = 0; j < 3; j++) {
            float edge2 = glm::length2(edges[j]);

            float ae = edge2 * (-glm::length2(vel)) +
                       glm::pow(glm::dot(edges[j], vel), 2.0f);
            float be = edge2 * 2.0f * glm::dot(vel, btv[j]) -
                       2.0f * (glm::dot(edges[j], vel) * glm::dot(edges[j], btv[j]));
            float ce = edge2 * (1.0f - glm::length2(btv[j])) + 
                       glm::pow(glm::dot(edges[j], btv[j]), 2.0f);
            float re;
            if (getLowestRoot(ae, be, ce, 10000.0f, re)) {
                float f0 = (glm::dot(edges[j], vel) * re - glm::dot(edges[j], btv[j])) /
                           glm::length2(edges[j]);
                float intDist = re * glm::length(vel);
                if (f0 >= 0.0f && f0 <= 1.0f && intDist < intersectionDistance) {
                    intersectionDistance = intDist;
                    intersectionPoint = tris[i + j] + f0 * edges[j];
                }
            }
        }
        */
    }
    return intersectionDistance < std::numeric_limits<float>::infinity();
}