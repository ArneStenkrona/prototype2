#include "physics_system.h"

#include "src/game/scene/scene.h"

#include "src/util/physics_util.h"
#include "src/util/math_util.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_operation.hpp>
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
    uint32_t id = m_ellipsoids.size();
    m_ellipsoids.push_back(ellipsoid);
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
            bool in = physics_util::intersectLineSegmentTriangle(origin, origin + direction * maxDistance,
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


void PhysicsSystem::updateCharacterPhysics(CharacterPhysics * physics,
                                           Transform * transforms,
                                           size_t n) {
    size_t i = 0;
    while (i < n) {
        // movement
        collideCharacterwithWorld(physics, transforms, n, i, false);
        collideCharacterwithWorld(physics, transforms, n, i, true);
        ++i;
    }
}

void PhysicsSystem::collideCharacterwithWorld(CharacterPhysics * physics,
                                              Transform * transforms,
                                              size_t n,
                                              uint32_t characterIndex,
                                              bool gravity) {
    // unpack variables
    glm::vec3 & position = transforms[characterIndex].position;
    glm::vec3 & velocity = gravity ? physics[characterIndex].gravityVelocity : physics[characterIndex].velocity ;
    glm::vec3 const & radii = m_ellipsoids[physics[characterIndex].colliderID];
    glm::vec3 & groundNormal = physics[characterIndex].groundNormal;
    bool & isGrounded = physics[characterIndex].isGrounded;

    isGrounded = false;

    unsigned int iterations = 5;
    while (iterations != 0) {
        --iterations;
        // prepare output variables
        glm::vec3 intersectionPoint;
        float intersectionTime = std::numeric_limits<float>::max();
        glm::vec3 collisionNormal;
        bool collision = false;
        uint32_t otherCharacterIndex = -1; // = 2,147,483,647

        // only check collision if there is any velocity
        if (glm::length2(velocity) > 0.0f) {
            // collide with meshes
            // broad-phase query
            AABB eAABB = { position - radii, position + radii };
            eAABB += { position + velocity - radii, 
                    position + velocity + radii };
            prt::vector<uint32_t> colIDs; 
            m_aabbTree.query(eAABB, colIDs);
            if (!colIDs.empty()) {                
                glm::vec3 ip;
                float t;
                glm::vec3 cn;
                if (collideCharacterWithMeshes(position, velocity, radii, colIDs, 
                                            ip, t, cn)) {
                    collision = true;
                    intersectionPoint = ip * radii;
                    intersectionTime = t;
                    collisionNormal = cn;
                }
            }

            // collide with other characters
            float t;
            glm::vec3 ip;
            glm::vec3 cn;
            uint32_t oci;
            // collide with other characters
            if (collideCharacterWithCharacters(physics, transforms, n, characterIndex, ip, t, cn, oci) &&
                t < intersectionTime) {
                collision = true;
                intersectionTime = t;
                intersectionPoint = ip;
                collisionNormal = cn;

                otherCharacterIndex = oci;
            }
        }
        // respond
        collisionResponse(collision, position, velocity, 
                          intersectionPoint, collisionNormal, intersectionTime);
        if (collision) { 
            // if collision with other character, we need to update that as well
            if (otherCharacterIndex != uint32_t(-1)) {
                collisionResponse(collision, 
                                transforms[otherCharacterIndex].position,
                                physics[otherCharacterIndex].velocity,
                                intersectionPoint,
                                -collisionNormal, // temporary hack: technically we need to compute the actual collision normal
                                intersectionTime);
            }
            // set grounded
            bool groundCollision = glm::dot(collisionNormal, glm::vec3{0.0f,1.0f,0.0f}) > 0.0f;
            isGrounded = isGrounded || groundCollision;

            if (groundCollision) {
                groundNormal = collisionNormal;
            }
        } else {
            // no collision ths iteration, we can end collision detection
            break;
        }
    }
}

/**
 * based on "Improved Collision detection and Response" by
 * Kasper Fauerby.
 * Link: http://www.peroxide.dk/papers/collision/collision.pdf
 */
bool PhysicsSystem::collideCharacterWithMeshes(glm::vec3 const & position, 
                                               glm::vec3 const & velocity, 
                                               glm::vec3 const & ellipsoidRadii,
                                               prt::vector<uint32_t> const & colliderIDs,
                                               glm::vec3 & intersectionPoint,
                                               float & intersectionTime,
                                               glm::vec3 & collisionNormal) {
    // convert input to ellipsoid space
    glm::vec3 sourcePoint = position / ellipsoidRadii; 
    glm::vec3 velocityVector = velocity / ellipsoidRadii;
    // collisioin normal in ellipsoid space
    glm::vec3 cn{1.0f};

    // construct all polygons
    struct Polygon {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
        glm::vec3 & operator[](size_t i) {  return *(&a + i); };
        glm::vec3 const & operator[](size_t i) const {  return *(&a + i); };
    };
    // count all indices
    size_t nIndices = 0;
    for (uint32_t colliderID : colliderIDs) {
        nIndices += m_meshColliders[colliderID].numIndices;
        
    }
    // fill vector with polygons in ellipsoid space
    prt::vector<Polygon> polygons;
    polygons.resize(nIndices / 3);
    glm::vec3* pCurr = &polygons[0].a;
    for (uint32_t colliderID : colliderIDs) {
        MeshCollider & meshCollider = m_meshColliders[colliderID];
        
        size_t endIndex = meshCollider.startIndex + meshCollider.numIndices;
        for (size_t i = meshCollider.startIndex; i < endIndex; ++i) {
            *pCurr = m_geometry_cache[i] / ellipsoidRadii;
            ++pCurr;
        }
    }
    
    intersectionTime = std::numeric_limits<float>::max();
    for (Polygon p : polygons) {
        // plane normal
        glm::vec3 n = glm::cross((p.b-p.a), (p.c-p.a));
        if (glm::length(n) == 0.0f) continue;
        n = glm::normalize(n);

        // skip triangle if relative velocity is not towards the triangle
        if (glm::length(velocityVector) > 0.0f && 
            glm::dot(glm::normalize(velocityVector), n) > 0.0f) {
            continue;
        }
        // plane constant
        float c = -glm::dot(n, p.a);

        float t0;
        float t1;
        float ndotv = glm::dot(n, velocityVector);
        float sigDist = glm::dot(sourcePoint, n) + c;
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
            t0 = glm::clamp(t0, 0.0f, 1.0f);
            t1 = glm::clamp(t1, 0.0f, 1.0f);
        }
        
        if (!embeddedInPlane) {
            // calculate plane intersection point;
            glm::vec3 pip = sourcePoint - n + t0 * velocityVector;
            // check if we are colliding inside the triangle
            if (physics_util::checkPointInTriangle(pip, p.a, p.b, p.c)) {
                float t = t0;
                if (t < intersectionTime) {
                    intersectionTime = t;
                    intersectionPoint = pip;
                    
                    glm::vec3 futurePos = sourcePoint + (intersectionTime * velocityVector);
                    cn = glm::normalize(futurePos - intersectionPoint);
                }
                // collision inside triangle
                continue;
            }
        }
        
        // sweep against vertices and edges
        // vertex;
        float av = glm::length2(velocityVector);
        if (av > 0.0f) {
            for (size_t j = 0; j < 3; j++) {
                float bv = 2.0f * glm::dot(velocityVector, sourcePoint - p[j]);
                float cv = glm::length2(p[j] - sourcePoint) - 1.0f;

                float sqrterm = (bv * bv) - (4.0f * av * cv);
                if (sqrterm >= 0.0f) {
                    float x1 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                    float x2 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                    float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                    if (t >= 0.0f && t <= 1.0f && t < intersectionTime) {
                        intersectionTime = t;
                        intersectionPoint = p[j];

                        glm::vec3 futurePos = sourcePoint + (intersectionTime * velocityVector);
                        cn = glm::normalize(futurePos - intersectionPoint);
                    }
                }
            }
        
            // edges
            glm::vec3 edges[3] = { p.b - p.a, p.c - p.b, p.a - p.c };
            glm::vec3 btv[3] = { p.a - sourcePoint, p.b - sourcePoint, p.c - sourcePoint }; // base to vertex
            for (size_t j = 0; j < 3; ++j) {
                float edge2 = glm::length2(edges[j]);

                float ae = edge2 * (-glm::length2(velocityVector)) +
                        glm::pow(glm::dot(edges[j], velocityVector), 2.0f);
                float be = edge2 * 2.0f * glm::dot(velocityVector, btv[j]) -
                        2.0f * (glm::dot(edges[j], velocityVector) * glm::dot(edges[j], btv[j]));
                float ce = edge2 * (1.0f - glm::length2(btv[j])) + 
                        glm::pow(glm::dot(edges[j], btv[j]), 2.0f);

                float sqrterm = (be * be) - (4.0f * ae * ce);
                if (ae != 0.0f && sqrterm >= 0.0f) {
                    float x1 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float x2 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                    if (t >= 0.0f && t <= 1.0f) {
                        float f0 = (glm::dot(edges[j], velocityVector) * t - glm::dot(edges[j], btv[j])) /
                                    glm::length2(edges[j]);
                        if (f0 >= 0.0f && f0 <= 1.0f && t < intersectionTime) {
                            intersectionTime = t;
                            intersectionPoint = p[j] + f0 * edges[j];

                            glm::vec3 futurePos = sourcePoint + (intersectionTime * velocityVector);
                            cn = glm::normalize(futurePos - intersectionPoint);
                        }
                    }
                }
            }
        }
    }
    // convert collision normal to original space
    collisionNormal = glm::normalize(glm::mat3(glm::inverse(glm::transpose(glm::scale(ellipsoidRadii)))) * cn);
    return 0.0f <= intersectionTime && intersectionTime <= 1.0f;
}

bool PhysicsSystem::collideCharacterWithCharacters(CharacterPhysics * physics,
                                                   Transform * transforms,
                                                   size_t n,
                                                   uint32_t characterIndex,
                                                   glm::vec3 & intersectionPoint,
                                                   float & intersectionTime,
                                                   glm::vec3 & collisionNormal,
                                                   uint32_t & otherCharacterIndex) {
    bool collision = false;
    // TODO: implement a breadth first so that we can avoid
    // O(n^2) when collision is checked for all characters
    intersectionTime = std::numeric_limits<float>::max();
    for (size_t i = 0; i < n; ++i) {
        if (i == characterIndex) continue;
        float t;
        glm::vec3 ip;
        glm::vec3 cn;
        if (collideEllipsoids(m_ellipsoids[physics[characterIndex].colliderID],
                          transforms[characterIndex].position,
                          physics[characterIndex].velocity,
                          m_ellipsoids[physics[i].colliderID],
                          transforms[i].position,
                          physics[i].velocity,
                          t,
                          ip,
                          cn) && t < intersectionTime) {
            intersectionTime = t;
            intersectionPoint = ip;
            collisionNormal = cn;
            otherCharacterIndex = i;
            collision = true;
        }

    }
    return collision;
}

// thank you David Eberly: https://www.geometrictools.com/Documentation/IntersectionSweptEllipsesEllipsoids.pdf
bool PhysicsSystem::collideEllipsoids(glm::vec3 const & ellipsoid0, 
                                      glm::vec3 const & sourcePoint0, 
                                      glm::vec3 const & velocity0, 
                                      glm::vec3 const & ellipsoid1, 
                                      glm::vec3 const & sourcePoint1, 
                                      glm::vec3 const & velocity1,
                                      float & intersectionTime, 
                                      glm::vec3 & intersectionPoint,
                                      glm::vec3 & collisionNormal) {
    // Get the parameters of ellipsoid0. 
    glm::vec3 K0 = sourcePoint0; 
    glm::mat3 R0(1.0f); // axis aligned => axis is identity matrix
    // glm::mat3 D0 = glm::diagonal3x3(glm::vec3(1.0f / (ellipsoid0[0] * ellipsoid0[0]), 
    //                                           1.0f / (ellipsoid0[1] * ellipsoid0[1]), 
    //                                           1.0f / (ellipsoid0[2] * ellipsoid0[2])));

    // Get the parameters of ellipsoid1. 
    glm::vec3 K1 = sourcePoint1; 
    glm::mat3 R1(1.0f); // axis aligned => axis is identity matrix
    glm::mat3 D1 = glm::diagonal3x3(glm::vec3(1.0f / (ellipsoid1[0] * ellipsoid1[0]), 
                                              1.0f / (ellipsoid1[1] * ellipsoid1[1]), 
                                              1.0f / (ellipsoid1[2] * ellipsoid1[2])));

    // Compute K2.
    glm::mat3 D0NegHalf = glm::diagonal3x3(glm::vec3(ellipsoid0[0], 
                                                     ellipsoid0[1],
                                                     ellipsoid0[2]));

    glm::mat3 D0Half = glm::diagonal3x3(glm::vec3(1.0f/ellipsoid0[0], 
                                                  1.0f/ellipsoid0[1],
                                                  1.0f/ellipsoid0[2]));

    glm::vec3 K2 = D0Half*((K1 - K0)*R0);
    // Compute M2.
    glm::mat3 R1TR0D0NegHalf = glm::transpose(R1) * (R0 * D0NegHalf); 
    glm::mat3 M2 = glm::transpose(R1TR0D0NegHalf) * (D1) * R1TR0D0NegHalf;
    // Factor M2 = R*D*R^T. 
    glm::mat3 Q = math_util::diagonalizer(M2);
    glm::mat3 D = glm::transpose(Q * M2 * glm::transpose(Q));
    glm::mat3 R = glm::transpose(Q);
    // Compute K. 
    glm::vec3 K = K2 * R;
    // Compute W.
    glm::vec3 W = (D0Half  * ((velocity1 - velocity0) * R0)) * R;
    // Transformed ellipsoid0 is Z^T*Z = 1 and transformed ellipsoid1 is 
    // (Z-K)^T*D*(Z-K) = 0.

    // Compute the initial closest point. 
    glm::vec3 P0;
    if (computeClosestPointEllipsoids(D, K, P0) >= 0.0f) {
        // The ellipsoid contains the origin, so the ellipsoids were not 
        // separated.
        return false;
    }
    
    double dist0 = glm::dot(P0, P0) - 1.0f;
    if (dist0 < 0.0) {
        // The ellipsoids are not separated.
        return false;
    }
    glm::vec3 zContact;
    if (!computeContactEllipsoids(D, K, W, intersectionTime, zContact)) {
        return false; 
    }
    // Transform contactPoint back to original space
    intersectionPoint = K0 + R0 * D0NegHalf * R * zContact;
    // collision normal is the gradient 2D(P - K)
    collisionNormal = glm::normalize(D1 * (intersectionPoint - K1));
    return intersectionTime <= 1.0f;//true;
}

// thank you David Eberly: https://www.geometrictools.com/Documentation/IntersectionSweptEllipsesEllipsoids.pdf
bool PhysicsSystem::computeContactEllipsoids(glm::mat3 const & D, 
                                             glm::vec3 const & K, 
                                             glm::vec3 const & W, 
                                             float & intersectionTime, 
                                             glm::vec3 & zContact) {
    float d0 = D[0][0], d1 = D[1][1], d2 = D[2][2];
    // float k0 = K[0], k1 = K[1], k2 = K[2]; 
    float w0 = W[0], w1 = W[1], w2 = W[2];

    static constexpr int maxIterations = 128; 
    static constexpr float epsilon = 1e-08f;
    intersectionTime = 0.0;
    for (int i = 0; i < maxIterations; ++i) {
        D[2][2];
        // Compute h(t).
        glm::vec3 Kmove = K + intersectionTime * W;
        float s = computeClosestPointEllipsoids(D, Kmove, zContact); 
        float tmp0 = d0 * Kmove[0] * s / (d0 * s - 1.0f);
        float tmp1 = d1 * Kmove[1] * s / (d1 * s - 1.0f);
        float tmp2 = d2 * Kmove[2] * s / (d2 * s - 1.0f);
        float h = tmp0 * tmp0 + tmp1 * tmp1 + tmp2 * tmp2 - 1.0f;
        if (fabs(h) < epsilon) {
            // We have found a root.
            return true; 
        }
        // Compute h’(t).
        float hder = 4.0f * tmp0 * w0 + 4.0f * tmp1 * w1 + 4.0f * tmp2 * w2; 
        if (hder > 0.0f) {
            // The ellipsoid cannot intersect the sphere.
            return false; 
        }
        // Compute the next iterate tNext = t - h(t)/h’(t).
        intersectionTime -= h/hder; 
    }
    computeClosestPointEllipsoids(D, K + intersectionTime * W, zContact);
    return true; 
}

// thank you David Eberly: https://www.geometrictools.com/Documentation/IntersectionSweptEllipsesEllipsoids.pdf
float PhysicsSystem::computeClosestPointEllipsoids(glm::mat3 const & D, 
                                                   glm::vec3 const & K, 
                                                   glm::vec3 & closestPoint) {
    float d0 = D[0][0], d1 = D[1][1], d2 = D[2][2];
    float k0 = K[0], k1 = K[1], k2 = K[2];
    float d0k0 = d0 * k0; 
    float d1k1 = d1 * k1; 
    float d2k2 = d2 * k2; 
    float d0k0k0 = d0k0 * k0; 
    float d1k1k1 = d1k1 * k1; 
    float d2k2k2 = d2k2 * k2;

    if (d0k0k0 + d1k1k1 + d2k2k2 - 1.0f < 0.0f) {
        // The ellipsoid contains the origin, so the ellipsoid and sphere are 
        // overlapping.
        return FLT_MAX;
    }

    static constexpr int maxIterations = 128; 
    static constexpr float epsilon = 1e-08f;

    float s = 0.0; 
    int i;
    for (i = 0; i < maxIterations; ++i) {
        // Compute f(s).
        float tmp0 = d0 * s - 1.0f;
        float tmp1 = d1 * s - 1.0f;
        float tmp2 = d2 * s - 1.0f;
        float tmp0sqr = tmp0 * tmp0;
        float tmp1sqr = tmp1 * tmp1;
        float tmp2sqr = tmp2 * tmp2;
        float f = d0k0k0 / tmp0sqr + d1k1k1 / tmp1sqr + d2k2k2 / tmp2sqr - 1.0f;
        if (fabs(f) < epsilon) {
            // We have found a root.
            break; 
        }
        // Compute f’(s).
        float tmp0cub = tmp0 * tmp0sqr;
        float tmp1cub = tmp1 * tmp1sqr;
        float tmp2cub = tmp2 * tmp2sqr;
        float fder = -2.0*(d0 * d0k0k0/tmp0cub + d1 * d1k1k1 / tmp1cub
                            + d2 * d2k2k2 / tmp2cub);
        // Compute the next iterate sNext = s - f(s)/f’(s).
        s -= f/fder;
    }
    closestPoint[0] = d0k0*s/(d0*s - 1.0); 
    closestPoint[1] = d1k1*s/(d1*s - 1.0); 
    closestPoint[2] = d2k2*s/(d2*s - 1.0); 
    return s;
}

void PhysicsSystem::respondCharacter(glm::vec3& position,
                                     glm::vec3& velocity,
                                     glm::vec3 const & intersectionPoint,
                                     float const intersectionTime) {
    static constexpr float verySmallDistance = 0.005f;

    position += intersectionTime * velocity;
    glm::vec3 slideNormal = glm::normalize(position - intersectionPoint);

    // by pushing out the character along the slide normal we gain
    // tolerance against small numerical errors
    position += verySmallDistance * slideNormal;
    // character should not slide on certain conditons
    // TODO: create a more robust condition for excluding slide
    if (glm::dot(glm::normalize(velocity), glm::vec3{0.0f,-1.0f,0.0f}) > 0.95f) {
        velocity = glm::vec3{0.0f};
    } else {
        velocity = glm::cross(slideNormal, 
                                glm::cross(velocity * (1.0f - intersectionTime), slideNormal));
    }
}

void PhysicsSystem::collisionResponse(bool collision,
                                      glm::vec3 & position,
                                      glm::vec3 & velocity,
                                      glm::vec3 const & /*intersectionPoint*/,
                                      glm::vec3 const & collisionNormal,
                                      float const intersectionTime) {
    if (glm::length2(velocity) == 0.0f) return;
        
    if (collision) {
        static constexpr float verySmallDistance = 0.005f;

        position += intersectionTime * velocity;
        glm::vec3 slideNormal = collisionNormal;//glm::normalize(position - intersectionPoint);

        // by pushing out the character along the slide normal we gain
        // tolerance against small numerical errors
        position += verySmallDistance * slideNormal;
        // character should not slide on certain conditons
        // TODO: create a more robust condition for excluding slide
        if (glm::dot(glm::normalize(velocity), glm::vec3{0.0f,-1.0f,0.0f}) > 0.95f) {
            velocity = glm::vec3{0.0f};
        } else {
            velocity = glm::cross(slideNormal, 
                                    glm::cross(velocity * (1.0f - intersectionTime), slideNormal));
        }
    } else {
        position += velocity;
    }
}
