#include "physics_system.h"

#include "src/game/scene/scene.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>

#include <dirent.h>

PhysicsSystem::PhysicsSystem(ModelManager & modelManager) 
    : m_modelManager{modelManager}
{
}

void PhysicsSystem::updateModelColliders(uint32_t const * colliderIDs,
                                        Transform const *transforms,
                                        size_t count) {
    for (auto & meshCollider : m_meshColliders) {
        meshCollider.hasMoved = false;
    }

    for (size_t i = 0; i< count; ++i) {
        MeshCollider * curr = m_meshColliders.data() + m_modelColliders[colliderIDs[i]].startIndex;
        MeshCollider * end = curr + m_modelColliders[colliderIDs[i]].numIndices;
        glm::mat4 mat = transforms[i].transformMatrix();
        while (curr < end) {
            curr->hasMoved = true;
            curr->transform = transforms[i];

            // update geometry cache
            size_t currIndex = curr->startIndex;
            size_t endIndex = currIndex + curr->numIndices;
            while (currIndex < endIndex) {
                m_geometry_cache[currIndex] = mat * glm::vec4(m_geometry[currIndex], 1.0f);
                ++currIndex;
            }
            ++curr;
        }
    }
}

void PhysicsSystem::resolveEllipsoidsModels(uint32_t const * ellipsoidIDs,
                                            Transform* ellipsoidTransforms,
                                            glm::vec3* ellipsoidVelocities,
                                            bool* ellipsoidsAreGrounded,
                                            glm::vec3* ellipsoidGroundNormals,
                                            size_t const nEllipsoids,
                                            uint32_t const * colliderIDs,
                                            size_t const nColliderIDs,
                                            float /*deltaTime*/){
    // this is currently O(m*n), which is pretty bad.
    // Spatial partitioning will be necessary for bigger scenes
    for (size_t i = 0; i < nEllipsoids; i++) {
        Transform& eT = ellipsoidTransforms[i];
        glm::vec3 const & eCol = ellipsoids[ellipsoidIDs[i]];
        glm::vec3& eVel = ellipsoidVelocities[i];
        bool & eIsGround = ellipsoidsAreGrounded[i];
        eIsGround = false;
        glm::vec3& eGroundN = ellipsoidGroundNormals[i];

        static constexpr size_t max_iter = 5;
        size_t iter = 0;
        while (iter < max_iter) {
            for (size_t j = 0; j < nColliderIDs; ++j) {
                // iterate through mesh colliders
                ModelCollider & mCol = m_modelColliders[colliderIDs[j]];
                prt::vector<uint32_t> colIDs;
                size_t curr = mCol.startIndex;
                size_t end = mCol.numIndices;
                while (curr < end) {
                    colIDs.push_back(curr);
                    ++curr;
                }

                glm::vec3 intersectionPoint;
                float intersectionTime;

                collideAndRespondEllipsoidMesh(eCol, eT, eVel, eIsGround, eGroundN,
                                               colIDs, intersectionPoint, intersectionTime);
            }
            ++iter;
        }
        eT.position += eVel;
        ellipsoidVelocities[i] = eVel;
    }    
}

void PhysicsSystem::addModelColliders(uint32_t const * modelIDs, Transform const * transforms, 
                                      size_t count, uint32_t * ids) {
    for (size_t i = 0; i < count; ++i) {
        ids[i] = addMeshCollider(m_modelManager.getModel(modelIDs[i]),
                                 transforms[i]);
    }
}

uint32_t PhysicsSystem::addEllipsoidCollider(const glm::vec3& ellipsoid) {
    uint32_t id = ellipsoids.size();
    ellipsoids.push_back(ellipsoid);
    return id;
}

uint32_t PhysicsSystem::addMeshCollider(Model const & model, Transform const & transform) {
    // get next index of geometry container
    size_t i = m_geometry.size();
    // insert new model collider
    size_t modelIndex = m_modelColliders.size();
    m_modelColliders.push_back({m_meshColliders.size(), model.meshes.size()});
    // resize geometry container
    m_geometry.resize(m_geometry.size() + model.indexBuffer.size());
    m_geometry_cache.resize(m_geometry.size() + model.indexBuffer.size());
    // create colliders from meshes
    for (auto const & mesh : model.meshes) {
        size_t index = mesh.startIndex;
        size_t endIndex = index + mesh.numIndices;
        // insert new mesh collider
        m_meshColliders.push_back({});
        MeshCollider & col = m_meshColliders.back();
        col.transform = transform;
        col.startIndex = i;
        col.numIndices = mesh.numIndices;

        glm::mat4 mat = transform.transformMatrix();
        while (index < endIndex) {
            m_geometry[i] = model.vertexBuffer[model.indexBuffer[index]].pos;
            m_geometry_cache[i] = mat * glm::vec4(m_geometry[i], 1.0f);
            ++i;
            ++index;
        }
    }
    return modelIndex;
}



bool PhysicsSystem::collideAndRespondEllipsoidMesh(glm::vec3 const & ellipsoid, 
                                                   Transform & ellipsoidTransform,
                                                   glm::vec3 & ellipsoidVel,
                                                   bool & ellipsoidIsGrounded,
                                                   glm::vec3& ellipsoidGroundNormal,
                                                //    MeshCollider const & meshCollider,
                                                   prt::vector<uint32_t> const & colliderIDs,
                                                   glm::vec3 & intersectionPoint,
                                                   float & intersectionTime) {
    bool collision = false;
    bool eGrounded = false;
    glm::vec3 eGroundN;
    intersectionTime = std::numeric_limits<float>::infinity();
    for (auto const & id : colliderIDs) {
        MeshCollider & meshCollider = m_meshColliders[id];
        glm::vec3 cbm = glm::vec3{ 1.0f / ellipsoid.x,
                                1.0f / ellipsoid.y,
                                1.0f / ellipsoid.z };
        
        prt::vector<glm::vec3> geometry;
        geometry.resize(meshCollider.numIndices);
        size_t index = meshCollider.startIndex;
        for (size_t i = 0; i < meshCollider.numIndices; ++i) {
            geometry[i] = cbm * m_geometry_cache[index];
            ++index;
        }

        // glm::vec3 tPos = { cbm.x * meshTransform.position.x, cbm.y * meshTransform.position.y, cbm.z * meshTransform.position.z };
        glm::vec3 tPos = { cbm.x * 0.0f, cbm.y * 0.0f, cbm.z * 0.0f };
        // glm::vec3 tVel = { cbm.x * trianglesVel.x, cbm.y * trianglesVel.y, cbm.z * trianglesVel.z };
        glm::vec3 tVel = { cbm.x * 0.0f, cbm.y * 0.0f, cbm.z * 0.0f };

        // ellipsoidIsGrounded = false;
        // ellipsoidGroundNormal = glm::vec3{0.0f,-1.0f,0.0f};
        float iTime;
        bool grounded;
        glm::vec3 groundNormal;
        bool res = collideEllipsoidMesh(ellipsoid, ellipsoidTransform.position, ellipsoidVel, 
                                        grounded, groundNormal,
                                        geometry, tPos, tVel,
                                        intersectionPoint, iTime);

        if (res && iTime < intersectionTime) {
            collision = true;
            eGrounded = grounded;
            eGroundN = groundNormal;
            intersectionTime = iTime;
        }
    }
        
    if (!collision) {
        return false;
    } else {
        respondEllipsoidMesh(ellipsoidTransform.position, ellipsoidVel,
                             intersectionPoint, intersectionTime);
        ellipsoidIsGrounded = ellipsoidIsGrounded || eGrounded;
        if (eGrounded) {
            ellipsoidGroundNormal = eGroundN;
            // ellipsoidGroundNormal = glm::dot(eGroundN, glm::vec3{0.0f,1.0f,0.0f}) >
            //                         glm::dot(ellipsoidGroundNormal, glm::vec3{0.0f,1.0f,0.0f}) ?
            //                         eGroundN : ellipsoidGroundNormal;
        }
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
bool PhysicsSystem::collideEllipsoidMesh(const glm::vec3& /*ellipsoid*/, 
                                         const glm::vec3& ellipsoidPos,
                                         const glm::vec3& ellipsoidVel,
                                         bool& isGrounded,
                                         glm::vec3& ellipsoidGroundNormal,
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
    glm::vec3 normal;
    for (size_t i = 0; i < tris.size(); i+=3) {
        // find the triangle plane
        const glm::vec3& p1 = tris[i];
        const glm::vec3& p2 = tris[i+1];
        const glm::vec3& p3 = tris[i+2];
        // plane normal
        glm::vec3 n = glm::cross((p2-p1),(p3-p1));
        n = glm::normalize(n);

        // skip triangle if relative velocity is not towards the triangle
        if (glm::length(vel) > 0.0f && glm::dot(glm::normalize(vel), n) > 0.0f) {
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
            if (std::abs(sigDist) > 1.0f) {
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
            // by clamping with -0.1f we can correct
            // for some errors where collider was
            // erroneously pushed inside triangle mesh
            // the previous frame
            t0 = glm::clamp(t0, -0.1f, 1.0f);
            t1 = glm::clamp(t1, -0.1f, 1.0f);
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
                    
                    glm::vec3 futurePos = ellipsoidPos + (intersectionTime * ellipsoidVel);
                    normal = glm::normalize(futurePos - intersectionPoint);
                }
                // collision inside triangle
                continue;
            }
        }
        
        // sweep against vertices and edges
        // vertex;
        float av = glm::length2(vel);
        if (av > 0.0f) {
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

                        glm::vec3 futurePos = ellipsoidPos + (intersectionTime * ellipsoidVel);
                        normal = glm::normalize(futurePos - intersectionPoint);
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
                if (ae != 0.0f && sqrterm >= 0.0f) {
                    float x1 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float x2 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                    if (t >= 0.0f && t <= 1.0f) {
                        float f0 = (glm::dot(edges[j], vel) * t - glm::dot(edges[j], btv[j])) /
                                    glm::length2(edges[j]);
                        if (f0 >= 0.0f && f0 <= 1.0f && t < intersectionTime) {
                            intersectionTime = t;
                            intersectionPoint = tris[i + j] + f0 * edges[j] + trianglesPos;

                            glm::vec3 futurePos = ellipsoidPos + (intersectionTime * ellipsoidVel);
                            normal = glm::normalize(futurePos - intersectionPoint);
                        }
                    }
                }
            }
        }
    }
    // isGrounded if slope is less than approx 45 degrees
    bool intersect = intersectionTime <= 1.0f;
    isGrounded = intersect && glm::dot(normal, glm::vec3{0.0f,1.0f,0.0f}) > 0.0f;
    ellipsoidGroundNormal = normal;
    return intersect;
}

void PhysicsSystem::respondEllipsoidMesh(glm::vec3& ellipsoidPos,
                                         glm::vec3& ellipsoidVel,
                                         glm::vec3& intersectionPoint,
                                         const float intersectionTime) {
    ellipsoidPos = ellipsoidPos + (intersectionTime * ellipsoidVel);
    glm::vec3 slideNormal = glm::normalize(ellipsoidPos - intersectionPoint);

    if (intersectionTime < 0.0f) slideNormal = -slideNormal;
    ellipsoidPos += verySmallDistance * slideNormal;
    ellipsoidVel = glm::cross(slideNormal, 
                              glm::cross(ellipsoidVel * (1.0f - intersectionTime), slideNormal));
}
