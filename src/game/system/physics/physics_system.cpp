#include "physics_system.h"

#include "src/game/scene/scene.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>

#include <dirent.h>

PhysicsSystem::PhysicsSystem(ModelManager & modelManager) 
    : m_modelManager{modelManager}
{
}

void PhysicsSystem::addModelColliders(uint32_t const * modelIDs, Transform const * transforms, 
                                      size_t count, uint32_t * ids) {
    for (size_t i = 0; i < count; ++i) {
        ids[i] = addModelCollider(m_modelManager.getNonAnimatedModel(modelIDs[i]),
                                 transforms[i]);
    }
    size_t prevSize = m_treeIndices.size();
    size_t numMesh = m_meshColliders.size() - prevSize;
    m_treeIndices.resize(prevSize + numMesh);
    prt::vector<uint32_t> colliderIDs;
    for (size_t i = prevSize; i < prevSize + numMesh; ++i) {
        colliderIDs.push_back(i);
    }
    m_aabbTree.insert(colliderIDs.data(), m_aabbs.data() + prevSize, numMesh, m_treeIndices.data() + prevSize);
}

uint32_t PhysicsSystem::addEllipsoidCollider(const glm::vec3& ellipsoid) {
    uint32_t id = ellipsoids.size();
    ellipsoids.push_back(ellipsoid);
    return id;
}

uint32_t PhysicsSystem::addModelCollider(Model const & model, Transform const & transform) {
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

        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        glm::mat4 mat = transform.transformMatrix();
        while (index < endIndex) {
            m_geometry[i] = model.vertexBuffer[model.indexBuffer[index]].pos;
            m_geometry_cache[i] = mat * glm::vec4(m_geometry[i], 1.0f);
            min = glm::min(min, m_geometry_cache[i]);
            max = glm::max(max, m_geometry_cache[i]);
            ++i;
            ++index;
        }
        m_aabbs.push_back({min, max});
    }
    return modelIndex;
}

void PhysicsSystem::updateModelColliders(uint32_t const * colliderIDs,
                                        Transform const *transforms,
                                        size_t count) {
    for (auto & meshCollider : m_meshColliders) {
        meshCollider.hasMoved = false;
    }
    prt::vector<int32_t> treeIndices;
    for (size_t i = 0; i< count; ++i) {
        size_t currIndex = m_modelColliders[colliderIDs[i]].startIndex;
        size_t endIndex = currIndex + m_modelColliders[colliderIDs[i]].numIndices;
        glm::mat4 mat = transforms[i].transformMatrix();
        while (currIndex < endIndex) {
            MeshCollider & curr = m_meshColliders[currIndex];
            curr.hasMoved = true;
            curr.transform = transforms[i];

            // update geometry cache
            size_t currIndex = curr.startIndex;
            size_t endIndex = currIndex + curr.numIndices;
            glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
            while (currIndex < endIndex) {
                m_geometry_cache[currIndex] = mat * glm::vec4(m_geometry[currIndex], 1.0f);
                min = glm::min(min, m_geometry_cache[currIndex]);
                max = glm::max(max, m_geometry_cache[currIndex]);
                ++currIndex;
            }
            m_aabbs[currIndex].lowerBound = min;
            m_aabbs[currIndex].upperBound = max;

            treeIndices.push_back(m_treeIndices[currIndex]);
            ++currIndex;
        }
    }
    
    m_aabbTree.update(treeIndices.data(), m_aabbs.data(), count);
}

// The collision detection entry point
void PhysicsSystem::collisionDetection(glm::vec3 & ellipsoidPosition,
                                       glm::vec3 & ellipsoidVelocity,
                                       glm::vec3 const & ellipsoidRadii) {
    // We need to do any pre-collision detection work here. Such as adding // gravity to our velocity vector. We want to do it in this
    // separate routine because the following routine is recursive, and we // don't want to recursively add gravity.
    // Add gravity
    glm::vec3 velocityVector = m_gravity + ellipsoidVelocity;
    // At this point, we’ll scale our inputs to the collision routine
    glm::vec3 sourcePoint = ellipsoidPosition / ellipsoidRadii; 
    velocityVector /= ellipsoidRadii;

    // Whom might we collide with?
    AABB eAABB = { ellipsoidPosition - ellipsoidRadii, ellipsoidPosition + ellipsoidRadii };
    eAABB += { ellipsoidPosition + ellipsoidVelocity - ellipsoidRadii, 
               ellipsoidPosition + ellipsoidVelocity + ellipsoidRadii };
    prt::vector<uint32_t> colIDs; 
    m_aabbTree.query(eAABB, colIDs);

    // Okay! Time to do some collisions
    collideWithWorld(sourcePoint, velocityVector, ellipsoidRadii, colIDs); 
    // Our collisions are complete, un-scale the output 
    ellipsoidPosition = sourcePoint * ellipsoidRadii;
    ellipsoidVelocity = velocityVector * ellipsoidRadii;
}

// The collision detection’s recursive routine
void PhysicsSystem::collideWithWorld(glm::vec3 & sourcePoint, 
                                     glm::vec3 & velocityVector, 
                                     glm:: vec3 ellipsoidRadii,
                                     prt::vector<uint32_t> const & colliderIDs) {
    // Check if list of potential colliders is empty
    if (colliderIDs.empty()) {
        sourcePoint += velocityVector;
        return;
    }
        
    // How far do we need to go?
    float distanceToTravel = glm::length(velocityVector);
    // Do we need to bother?
    if (distanceToTravel == 0.0f) return;
    // You’ll need to write this routine to deal with your specific data scale_potential_colliders_to_ellipsoid_space(radiusVector);
    // Determine the nearest collider from the list potentialColliders
    bool collisionFound = false;
    float nearestDistance = -1.0f;
    glm::vec3 nearestIntersectionPoint;
    glm::vec3 nearestPolygonIntersectionPoint;

    // construct all polygons
    struct Polygon {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
    };
    // count all indices
    size_t nIndices = 0;
    for (uint32_t colliderID : colliderIDs) {
        nIndices += m_meshColliders[colliderID].numIndices;
        
    }
    // fill vector with polygons in ellipsoid space
    prt::vector<Polygon> polygons;
    polygons.resize(nIndices / 3);
    for (uint32_t colliderID : colliderIDs) {
        MeshCollider & meshCollider = m_meshColliders[colliderID];
        
        size_t endIndex = meshCollider.startIndex + meshCollider.numIndices;
        glm::vec3* pStart = &polygons[0].a;
        for (size_t i = meshCollider.startIndex; i < endIndex; ++i) {
            *pStart = m_geometry_cache[i] / ellipsoidRadii;
            ++pStart;
        }
    }
    
    for (Polygon p : polygons) {
        // Plane origin/normal
        glm::vec3 pOrigin = p.a;
        glm::vec3 pNormal = glm::cross((p.b - p.a), (p.c - p.a));
        if (glm::length2(pNormal) == 0) {
            continue;
        } else {
            pNormal = glm::normalize(pNormal);
        }
        // Determine the distance from the plane to the source
        float pDist = intersectRayPlane(pOrigin, pNormal, sourcePoint, -pNormal); 
        glm::vec3 sphereIntersectionPoint;
        glm::vec3 planeIntersectionPoint;
        // Is the source point behind the plane? 
        //
        // [note that you can remove this condition if your visuals are not
        // using backface culling]
        if (pDist < 0.0f) {
            continue;
        }
        // Is the plane embedded (i.e. within the distance of 1.0 for our
        // unit sphere)?
        else if (pDist <= 1.0f) {
            // Calculate the plane intersection point
            glm::vec3 temp = -pNormal * pDist; 
            planeIntersectionPoint = sourcePoint + temp;
        } else {
            // Calculate the sphere intersection point 
            sphereIntersectionPoint = sourcePoint - pNormal; // Calculate the plane intersection point
            float t = intersectRayPlane(pOrigin, pNormal, sphereIntersectionPoint, glm::normalize(velocityVector));
            // Are we traveling away from this polygon?
            if (t < 0.0) continue;
            // Calculate the plane intersection point
            glm::vec3 v = velocityVector * t; 
            planeIntersectionPoint = sphereIntersectionPoint + v;
        }
        // Unless otherwise noted, our polygonIntersectionPoint is the 
        // same point as planeIntersectionPoint
        glm::vec3 polygonIntersectionPoint = planeIntersectionPoint;
        glm::vec3 bary = barycentric(planeIntersectionPoint, p.a, p.b, p.c);
        // check if plane intersection is within triangle
        if (!(0.0f < bary.x && bary.x < 1.0f &&
              0.0f < bary.y && bary.y < 1.0f &&
              0.0f < bary.z && bary.z < 1.0f)) {
            polygonIntersectionPoint = closestPointOnTrianglePerimeter(p.a, p.b, p.c, planeIntersectionPoint);
        }
        // Invert the velocity vector
        glm::vec3 negativeVelocityVector = -velocityVector;

        // Using the polygonIntersectionPoint, we need to reverse-intersect 
        // with the sphere (note: the 1.0 below is the unit-sphere’s radius)
        float t = intersectSphere(polygonIntersectionPoint, negativeVelocityVector, sourcePoint, 1.0f);
        // Was there an intersection with the sphere? 
        if (t >= 0.0 && t <= distanceToTravel) {
            // Where did we intersect the sphere?
            glm::vec3 v = negativeVelocityVector * t; 
            glm::vec3 intersectionPoint = polygonIntersectionPoint + v;
            // Closest intersection thus far?
            if (!collisionFound || t < nearestDistance) {
                nearestDistance = t;
                nearestIntersectionPoint = intersectionPoint; nearestPolygonIntersectionPoint =
                polygonIntersectionPoint; collisionFound = true;
            }
        }
    }
    // If we never found a collision, we can safely move to the destination // and bail
    if (!collisionFound) {
        sourcePoint += velocityVector;
        return; 
    }
    // Move to the nearest collision
    glm::vec3 v = glm::normalize(velocityVector) * (nearestDistance - /*EPSILON*/ 0.00001f); 
    sourcePoint += v;
    // What's our destination (relative to the point of contact)?
    v = glm::normalize(v) * (distanceToTravel - nearestDistance);
    glm::vec3 destinationPoint = nearestPolygonIntersectionPoint + v;
    // Determine the sliding plane
    glm::vec3 slidePlaneOrigin = nearestPolygonIntersectionPoint;
    glm::vec3 slidePlaneNormal = nearestPolygonIntersectionPoint - sourcePoint;
    // We now project the destination point onto the sliding plane
    float time = intersectRayPlane(slidePlaneOrigin, slidePlaneNormal, destinationPoint, slidePlaneNormal);
    slidePlaneNormal = glm::normalize(slidePlaneNormal) * time;
    glm::vec3 destinationProjectionNormal = slidePlaneNormal;
    glm::vec3 newDestinationPoint = destinationPoint + destinationProjectionNormal;
    // Generate the slide vector, which will become our new velocity vector // for the next iteration
    // glm::vec3 newVelocityVector = newDestinationPoint - nearestPolygonIntersectionPoint;
    velocityVector = newDestinationPoint - nearestPolygonIntersectionPoint;
    // Recursively slide (without adding gravity) 
    //collideWithWorld(sourcePoint, newVelocityVector);
}

void PhysicsSystem::resolveEllipsoidsModels(uint32_t const * ellipsoidIDs,
                                            Transform* ellipsoidTransforms,
                                            glm::vec3* ellipsoidVelocities,
                                            bool* ellipsoidsAreGrounded,
                                            glm::vec3* ellipsoidGroundNormals,
                                            size_t const nEllipsoids,
                                            float /*deltaTime*/){
    for (size_t i = 0; i < nEllipsoids; i++) {
        Transform& eT = ellipsoidTransforms[i];
        glm::vec3 const & eCol = ellipsoids[ellipsoidIDs[i]];
        glm::vec3& eVel = ellipsoidVelocities[i];
        bool & eIsGround = ellipsoidsAreGrounded[i];
        eIsGround = false;
        glm::vec3& eGroundN = ellipsoidGroundNormals[i];

        static constexpr size_t max_iter = 5;
        size_t iter = 0;

        AABB eAABB = { eT.position - eCol, eT.position + eCol };
        eAABB += { eT.position + eVel - eCol, eT.position +eVel + eCol };
        prt::vector<uint32_t> colIDs; 
        m_aabbTree.query(eAABB, colIDs);

        while (iter < max_iter) {
            glm::vec3 intersectionPoint;
            float intersectionTime;
            collideAndRespondEllipsoidMesh(eCol, eT, eVel, eIsGround, eGroundN,
                                           colIDs, intersectionPoint, intersectionTime);
            ++iter;
        }
        eT.position += eVel;
        ellipsoidVelocities[i] = eVel;
    }    
}

bool PhysicsSystem::collideAndRespondEllipsoidMesh(glm::vec3 const & ellipsoid, 
                                                   Transform & ellipsoidTransform,
                                                   glm::vec3 & ellipsoidVel,
                                                   bool & ellipsoidIsGrounded,
                                                   glm::vec3& ellipsoidGroundNormal,
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
        if (glm::length(n) == 0.0f) continue;
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
            t0 = glm::clamp(t0, -0.0f, 1.0f);
            t1 = glm::clamp(t1, -0.0f, 1.0f);
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

// From "Realtime Collision Detection" by Christer Ericson
bool PhysicsSystem::raycast(glm::vec3 const& origin,
                            glm::vec3 const& direction,
                            float maxDistance,
                            glm::vec3 & hit) {
    prt::vector<uint32_t> colIDs; 
    m_aabbTree.queryRaycast(origin, direction, maxDistance, colIDs);

    float intersectionTime = std::numeric_limits<float>::max();
    bool intersect = false;
    for (auto id : colIDs) {
        MeshCollider & meshCollider = m_meshColliders[id];
        size_t index = meshCollider.startIndex;
        for (size_t i = 0; i < meshCollider.numIndices; i+=3) {
            float t;
            bool in = intersectLineSegmentTriangle(origin, origin + direction * maxDistance,
                                                   m_geometry_cache[index],
                                                   m_geometry_cache[index+1],
                                                   m_geometry_cache[index+2],
                                                   t);
            intersect |= in;
            if (in) {
                intersectionTime = std::min(intersectionTime, t);
            }
            
            index += 3;
        }
    }
    if (intersect) {
        hit = origin + direction * intersectionTime * maxDistance;
    }
    return intersect;
}

// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
glm::vec3 PhysicsSystem::closestPointOnTrianglePerimeter(glm::vec3 const & a,
                                                         glm::vec3 const & b,
                                                         glm::vec3 const & c,
                                                         glm::vec3 const & p) {
    glm::vec3 rab = closestPointOnLine(a, b, p); 
    glm::vec3 rbc = closestPointOnLine(b, c, p); 
    glm::vec3 rca = closestPointOnLine(c, a, p);
    float distAB2 = glm::distance2(rab, p);
    float distBC2 = glm::distance2(rbc, p);
    float distCA2 = glm::distance2(rca, p);

    float minDist = distAB2;
    glm::vec3 minPoint = rab;
    if (distBC2 < minDist) {
        minDist = distBC2;
        minPoint = rbc;
    }
    if (distCA2 < minDist) {
        // minDist = distCA2;
        minPoint = rca;
    }
    return minPoint;
}


// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
glm::vec3 PhysicsSystem::closestPointOnLine(glm::vec3 const & a,
                                            glm::vec3 const & b,
                                            glm::vec3 const & p) {
    // Determine t (the length of the vector from ‘a’ to ‘p’)
    glm::vec3 c = p - a;
    glm::vec3 v = glm::normalize(b - a); 
    float d = glm::distance(a, b);
    float t = glm::dot(v, c);
    // Check to see if ‘t’ is beyond the extents of the line segment
    if (t < 0) return a; 
    if (t > d) return b;
    // Return the point between ‘a’ and ‘b’
    v = v * t; 
    return a + v;
}

// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
float PhysicsSystem::intersectRayPlane(glm::vec3 const & planeOrigin, 
                                       glm::vec3 const & planeNormal,
                                       glm::vec3 const & rayOrigin,
                                       glm::vec3 const & rayVector) {
    float d = -glm::dot(planeNormal, planeOrigin); 
    float numer = glm::dot(planeNormal, rayOrigin) + d; 
    float denom = glm::dot(planeNormal, rayVector); 
    return -(numer / denom);
}

// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
float PhysicsSystem::intersectSphere(glm::vec3 const & rO, 
                                     glm::vec3 const & rV, 
                                     glm::vec3 const & sO, 
                                     float sR) {
    glm::vec3 q = sO - rO;
    float c = glm::length(q);
    float v = glm::dot(q, rV);
    float d = sR*sR - (c * c - v * v);
    // If there was no intersection, return -1
    if (d < 0.0) return -1.0;
    // Return the distance to the [first] intersecting point 
    return v - sqrt(d);
}

// From "Realtime Collision Detection" by Christer Ericson
bool PhysicsSystem::intersectLineSegmentTriangle(glm::vec3 const & origin, 
                                                 glm::vec3 const & end, 
                                                 glm::vec3 const & a,
                                                 glm::vec3 const & b,
                                                 glm::vec3 const & c,
                                                 float &t) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 qp = origin - end;
    // Compute triangle normal. Can be precalculated or cached if // intersecting multiple segments against the same triangle 
    glm::vec3 n = glm::cross(ab, ac);
    // Compute denominator d. If d <= 0, segment is parallel to or points // away from triangle, so exit early
    float d = glm::dot(qp, n);
    if (d <= 0.0f) return false;
        // Compute intersection t value of pq with plane of triangle. A ray
        // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
        // dividing by d until intersection has been found to pierce triangle

    glm::vec3 ap = origin - a;
    t = glm::dot(ap, n);
    if (t < 0.0f) return false;
    if (t > d) return false; // For segment; exclude this code line for a ray test
    // Compute barycentric coordinate components and test if within bounds
    glm::vec3 e = glm::cross(qp, ap);
    float v = glm::dot(ac, e);
    if (v < 0.0f || v > d) return false;
    float w = -glm::dot(ab, e);
    if (w < 0.0f || v + w > d) return false;
    // Segment/ray intersects triangle. Perform delayed division and // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    // v *= ood;
    // w *= ood;
    //float u = 1.0f - v - w;
    return true;
}

// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
// From "Realtime Collision Detection" by Christer Ericson
glm::vec3 PhysicsSystem::barycentric(glm::vec3 const & p, 
                                     glm::vec3 const & a, 
                                     glm::vec3 const & b, 
                                     glm::vec3 const & c) {
    glm::vec3 bary{};
    glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    bary.y = (d11 * d20 - d01 * d21) / denom;
    bary.z = (d00 * d21 - d01 * d20) / denom;
    bary.x = 1.0f - bary.y - bary.z;
    return bary;
}
