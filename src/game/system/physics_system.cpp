#include "physics_system.h"

#include <glm/gtx/norm.hpp>

bool PhysicsSystem::collideAndRespondEllipsoidTriangles(const glm::vec3& ellipsoid, 
                                                        glm::vec3& ellipsoidPos,
                                                        glm::vec3& ellipsoidVel,
                                                        const prt::vector<glm::vec3>& triangles,
                                                        const glm::vec3& trianglesPos,
                                                        const glm::vec3& trianglesVel,
                                                        glm::vec3& intersectionPoint,
                                                        float& intersectionTime) {
    prt::vector<glm::vec3> tris = triangles;
    // convert to eSpace
    // change of basis matrix, though simplified as a vector
    glm::vec3 cbm = { 1.0f / ellipsoid.x, 1.0f / ellipsoid.y, 1.0f / ellipsoid.z };
    for (size_t i = 0; i < tris.size(); i++) {
        tris[i] = { cbm.x * tris[i].x, cbm.y * tris[i].y, cbm.z * tris[i].z };
    }
    glm::vec3 ePos = { cbm.x * ellipsoidPos.x, cbm.y * ellipsoidPos.y, cbm.z * ellipsoidPos.z };
    glm::vec3 eVel = { cbm.x * ellipsoidVel.x, cbm.y * ellipsoidVel.y, cbm.z * ellipsoidVel.z };
    
    glm::vec3 tPos = { cbm.x * trianglesPos.x, cbm.y * trianglesPos.y, cbm.z * trianglesPos.z };
    glm::vec3 tVel = { cbm.x * trianglesVel.x, cbm.y * trianglesVel.y, cbm.z * trianglesVel.z };

    static constexpr uint32_t max_iter = 10;
    uint32_t iter = 0;
    while (collideEllipsoidTriangles(ellipsoid,
                                     ePos,
                                     eVel,
                                     tris,
                                     tPos,
                                     tVel,
                                     intersectionPoint,
                                     intersectionTime) && iter < max_iter) {
        respondEllipsoidTriangles(ePos, eVel,
                                  intersectionPoint, intersectionTime);

        iter++;
    }   
    //assert(iter < max_iter);
    if (iter == 0) {
        // no collision, apply velocity as normal
        ellipsoidPos += ellipsoidVel;
        return false;
    } else {
        // convert result back to world space
        ellipsoidPos = { ePos.x * ellipsoid.x, ePos.y * ellipsoid.y, ePos.z * ellipsoid.z };
        ellipsoidVel = { eVel.x * ellipsoid.x, eVel.y * ellipsoid.y, eVel.z * ellipsoid.z };
        //ellipsoidPos += ellipsoidVel;
        return true;
    }
}

// thank you Gene for this concise implementation
// https://stackoverflow.com/a/25516767
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
/**
 * based on "Improved Collision detection and Response" by
 * Kasper Fauerby.
 * Link: http://www.peroxide.dk/papers/collision/collision.pdf
 */
bool PhysicsSystem::collideEllipsoidTriangles(const glm::vec3& /*ellipsoid*/, 
                                              const glm::vec3& ellipsoidPos,
                                              const glm::vec3& ellipsoidVel,
                                              const prt::vector<glm::vec3>& triangles,
                                              const glm::vec3& trianglesPos,
                                              const glm::vec3& trianglesVel,
                                              glm::vec3& intersectionPoint,
                                              float& intersectionTime) {
    const prt::vector<glm::vec3>& tris = triangles;
    glm::vec3 pos = ellipsoidPos - trianglesPos;
    glm::vec3 vel = ellipsoidVel - trianglesVel;

    // results of the collision test
    intersectionTime = std::numeric_limits<float>::infinity();

    for (size_t i = 0; i < tris.size(); i+=3) {
        // find the triangle plane
        const glm::vec3& p1 = tris[i];
        const glm::vec3& p2 = tris[i+1];
        const glm::vec3& p3 = tris[i+2];
        // plane normal
        glm::vec3 n = glm::cross((p2-p1),(p3-p1));
        n = glm::normalize(n);

        // skip triangle if relative velocity is not towards the triangle
        if (glm::dot(glm::normalize(vel), n) > 0.0f) {
            continue;
        }

        // plane constant
        float c = -glm::dot(n, p1);

        float t0;
        float t1;
        float ndotv = glm::dot(n, vel);
        float sigDist = glm::dot(pos, n) + c;
        bool embeddedInPlane = false;
        // check distance to plane and set t0,t1
        if (ndotv == 0.0f) {
            if (std::abs(sigDist) >= 1.0f) {
                continue;
            } else {
                // sphere is embedded in plane
                t0 = 0.0f;
                t1 = 1.0f;
                embeddedInPlane = true;
            }
        } else {
            t0 = (1.0f - sigDist) / ndotv;
            t1 = (-1.0f - sigDist) / ndotv;

            if (t0 > t1) {
                float temp = t0;
                t0 = t1;
                t1 = temp;
            }
            if ( t1 < 0.0f || t0 > 1.0f) {
                // collision occurs outside velocity range
                continue;
            }
            t0 = glm::clamp(t0, -1.0f, 1.0f);
            t1 = glm::clamp(t1, -1.0f, 1.0f);
        }
        
        if (!embeddedInPlane) {
            // calculate plane intersection point;
            glm::vec3 pip = pos - n + t0 * vel;
            // check if we are colliding inside the triangle
            if (checkPointInTriangle(pip, p1, p2, p3)) {
                float t = t0;
                if (t < intersectionTime) {
                    intersectionTime = t;
                    intersectionPoint = pip + trianglesPos;
                }
                // collision inside triangle
                continue;
            }
        }
        
        // sweep against vertices and edges
        // vertex;
        float av = glm::length2(vel);
        for (size_t j = 0; j < 3; j++) {
            float bv = 2.0f * glm::dot(vel, pos - tris[i + j]);
            float cv = glm::length2(tris[i + j] - pos) - 1.0f;

            float sqrterm = (bv * bv) - (4.0f * av * cv);
            if (sqrterm >= 0.0f) {
                float x1 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                float x2 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                if (t >= 0.0f && t <= 1.0f && t < intersectionTime) {
                    intersectionTime = t;
                    intersectionPoint = tris[i + j] + trianglesPos;
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

            float sqrterm = (be * be) - (4.0f * ae * ce);
            if (sqrterm >= 0.0f) {
                float x1 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                float x2 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                if (t >= 0.0f && t <= 1.0f) {
                    float f0 = (glm::dot(edges[j], vel) * t - glm::dot(edges[j], btv[j])) /
                                glm::length2(edges[j]);
                    if (f0 >= 0.0f && f0 <= 1.0f && t < intersectionTime) {
                        intersectionTime = t;
                        intersectionPoint = tris[i + j] + f0 * edges[j] + trianglesPos;
                    }
                }
            }
        }
        
    }
    return intersectionTime <= 1.0f;
}

void PhysicsSystem::respondEllipsoidTriangles(glm::vec3& ellipsoidPos,
                                              glm::vec3& ellipsoidVel,
                                              glm::vec3& intersectionPoint,
                                              const float intersectionTime) {
    ellipsoidPos = ellipsoidPos + (intersectionTime * ellipsoidVel);
    glm::vec3 slideNormal = glm::normalize(ellipsoidPos - intersectionPoint);
    if (intersectionTime < 0.0f) slideNormal = -slideNormal;
    ellipsoidPos += verySmallDistance * slideNormal;
    ellipsoidVel = glm::cross(slideNormal, 
                              glm::cross(ellipsoidVel * (1.0f - intersectionTime), slideNormal) 
                              / glm::length(slideNormal)) /
                              glm::length(slideNormal);
}