#include "physics_system.h"

#include "src/game/scene/scene.h"

#include "src/util/physics_util.h"
#include "src/util/math_util.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/string_cast.hpp>

#include <dirent.h>

PhysicsSystem::PhysicsSystem() {}

ColliderTag PhysicsSystem::addEllipsoidCollider(glm::vec3 const & ellipsoid) {
    assert(m_ellipsoids.size() < std::numeric_limits<uint16_t>::max() && "Too many ellipsoid colliders!");
    uint16_t id = m_ellipsoids.size();
    m_ellipsoids.push_back(ellipsoid);
    m_aabbData.ellipsoidIndices.push_back({});

    m_aabbData.ellipsoidAABBs.push_back({glm::vec3{0.0f}, ellipsoid});

    ColliderTag tag = { uint16_t(id), ColliderType::COLLIDER_TYPE_ELLIPSOID };
    m_aabbData.tree.insert(&tag, &m_aabbData.ellipsoidAABBs[id], 1, &m_aabbData.ellipsoidIndices[id]);

    return tag;
}

ColliderTag PhysicsSystem::addModelCollider(Model const & model, Transform const & transform) {
    ColliderTag tag;
    tag.type = COLLIDER_TYPE_MODEL;


    if (!m_models.freeList.empty()) {
        tag.index = m_models.freeList.back();
        m_models.freeList.pop_back();
    } else {
        tag.index = m_models.models.size();
        m_models.geometries.push_back({});
        m_models.models.push_back({});
    }
    ModelCollider & col = m_models.models[tag.index];

    col.startIndex = m_models.meshes.size();
    col.numIndices = model.meshes.size();

    Geometry & geometry = m_models.geometries[tag.index];
    geometry.raw.resize(model.indexBuffer.size());
    geometry.cache.resize(model.indexBuffer.size());

    unsigned int i = 0;

    for (Model::Mesh & mesh : model.meshes) {
        size_t index = mesh.startIndex;
        size_t endIndex = index + mesh.numIndices;

        m_models.meshes.push_back({});
        MeshCollider & mcol = m_models.meshes.back();

        mcol.transform = transform;
        mcol.startIndex = i;
        mcol.numIndices = mesh.numIndices;
        mcol.modelIndex = tag.index;

        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        glm::mat4 mat = transform.transformMatrix();
        while (index < endIndex) {
            geometry.raw[i] = model.vertexBuffer[model.indexBuffer[index]].pos;
            geometry.cache[i] = mat * glm::vec4(geometry.raw[i], 1.0f);
            min = glm::min(min, geometry.cache[i]);
            max = glm::max(max, geometry.cache[i]);
            ++i;
            ++index;
        }
        m_aabbData.meshAABBs.push_back({min, max});
    }
    size_t prevSize = m_aabbData.meshIndices.size();
    size_t numMesh = model.meshes.size();
    m_aabbData.meshIndices.resize(prevSize + numMesh);
    prt::vector<ColliderTag> tags;
    for (size_t i = prevSize; i < prevSize + numMesh; ++i) {
        assert(i < std::numeric_limits<ColliderIndex>::max() && "Too many mesh colliders!");
        tags.push_back({ColliderIndex(i), ColliderType::COLLIDER_TYPE_MESH});
    }
    m_aabbData.tree.insert(tags.data(), m_aabbData.meshAABBs.data() + prevSize, numMesh, m_aabbData.meshIndices.data() + prevSize);


    return tag;
}

void PhysicsSystem::removeCollider(ColliderTag const & tag) {
    switch (tag.type) {
        case COLLIDER_TYPE_MODEL:
            removeModelCollider(tag.index);
            break;
        case COLLIDER_TYPE_ELLIPSOID:
            assert(false && "Not yet implemeneted!");
            break;
        default:
            assert(false && "This collider type can not be removed!");

    }
}

void PhysicsSystem::removeModelCollider(ColliderIndex colliderIndex) {
    m_models.freeList.push_back(colliderIndex);

    ModelCollider & col = m_models.models[colliderIndex];
    unsigned int index = col.startIndex;
    unsigned int numIndices = col.numIndices;
    unsigned int endIndex = index + numIndices;

    for (ModelCollider & mc : m_models.models) {
        if (mc.startIndex > index) {
            mc.startIndex -= numIndices;
        }
    }

    m_aabbData.tree.remove(&m_aabbData.meshIndices[index], numIndices);

    if (endIndex < m_aabbData.meshIndices.size()) {
        m_aabbData.tree.update(&m_aabbData.meshIndices[endIndex], 
                               &m_aabbData.meshAABBs[endIndex], m_aabbData.meshIndices.size() - endIndex);
    }

    m_models.meshes.remove(index, numIndices);
    m_aabbData.meshAABBs.remove(index, numIndices);
    m_aabbData.meshIndices.remove(index, numIndices);

    col = ModelCollider{};
    m_models.geometries[index] = Geometry{};
}

void PhysicsSystem::updateEllipsoidCollider(ColliderTag const & tag, glm::vec3 const & dimensions) {
    assert(tag.type == COLLIDER_TYPE_ELLIPSOID);
    m_ellipsoids[tag.index] = dimensions;
}

void PhysicsSystem::updateModelColliders(ColliderTag const * tags,
                                         Transform const *transforms,
                                         size_t count) {
    for (auto & meshCollider : m_models.meshes) {
        meshCollider.hasMoved = false;
    }
    for (size_t i = 0; i < count; ++i) {
        assert(tags[i].type == COLLIDER_TYPE_MODEL);
        ModelCollider const & col = m_models.models[tags[i].index];
        Geometry & geometry = m_models.geometries[tags[i].index];

        size_t startIndex = col.startIndex;
        size_t currIndex = startIndex;
        size_t numIndices = col.numIndices;
        size_t endIndex = currIndex + numIndices;
        glm::mat4 mat = transforms[i].transformMatrix();

        while (currIndex < endIndex) {
            MeshCollider & curr = m_models.meshes[currIndex];
            curr.hasMoved = true;
            curr.transform = transforms[i];

            // update geometry cache
            size_t currIndex2 = curr.startIndex;
            size_t endIndex2 = currIndex2 + curr.numIndices;
            glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
            while (currIndex2 < endIndex2) {
                geometry.cache[currIndex2] = mat * glm::vec4(geometry.raw[currIndex2], 1.0f);
                min = glm::min(min, geometry.cache[currIndex2]);
                max = glm::max(max, geometry.cache[currIndex2]);
                ++currIndex2;
            }
            m_aabbData.meshAABBs[currIndex].lowerBound = min;
            m_aabbData.meshAABBs[currIndex].upperBound = max;
            ++currIndex;
        }

        m_aabbData.tree.update(&m_aabbData.meshIndices[startIndex], 
                               &m_aabbData.meshAABBs[startIndex], numIndices);
    }
}

// From "Realtime Collision Detection" by Christer Ericson
bool PhysicsSystem::raycast(glm::vec3 const& origin,
                            glm::vec3 const& direction,
                            float maxDistance,
                            glm::vec3 & hit) {
    prt::vector<ColliderTag> tags; 
    m_aabbData.tree.queryRaycast(origin, direction, maxDistance, tags);

    float intersectionTime = std::numeric_limits<float>::max();
    bool intersect = false;
    for (auto tag : tags) {
        MeshCollider & meshCollider = m_models.meshes[tag.index];

        Geometry & geometry = m_models.geometries[meshCollider.modelIndex];

        size_t index = meshCollider.startIndex;
        for (size_t i = 0; i < meshCollider.numIndices; i+=3) {
            float t;
            bool in = physics_util::intersectLineSegmentTriangle(origin, origin + direction * maxDistance,
                                                                 geometry.cache[index],
                                                                 geometry.cache[index+1],
                                                                 geometry.cache[index+2],
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


void PhysicsSystem::updateCharacterPhysics(float deltaTime,
                                           CharacterPhysics * physics,
                                           Transform * transforms,
                                           size_t n) {
    
    // update character aabb's
    prt::hash_map<uint16_t, size_t> tagToCharacter;

    size_t i = 0;    
    while (i < n) {
        AABB & eAABB = m_aabbData.ellipsoidAABBs[physics[i].colliderTag.index];
        // bounds = radii extents
        eAABB = { transforms[i].position - m_ellipsoids[physics[i].colliderTag.index], 
                  transforms[i].position + m_ellipsoids[physics[i].colliderTag.index] };
        // bounds += velocity extents
        eAABB += { transforms[i].position + physics[i].velocity + glm::vec3{0.0f, -1.0f, 0.0f} * m_gravity * deltaTime - m_ellipsoids[physics[i].colliderTag.index], 
                   transforms[i].position + physics[i].velocity + glm::vec3{0.0f, -1.0f, 0.0f} * m_gravity * deltaTime + m_ellipsoids[physics[i].colliderTag.index] };

        tagToCharacter.insert(physics[i].colliderTag.index, i);
        ++i;
    }
    m_aabbData.tree.update(m_aabbData.ellipsoidIndices.data(), m_aabbData.ellipsoidAABBs.data(), m_ellipsoids.size());
    
    // collide
    i = 0;
    while (i < n) {
        // movement
        bool grounded = false;
        collideCharacterwithWorld(physics, transforms, n, i, tagToCharacter, grounded);
        // add gravity
        physics[i].velocity += glm::vec3{0.0f, -1.0f, 0.0f} * m_gravity * deltaTime;

        collideCharacterwithWorld(physics, transforms, n, i, tagToCharacter, grounded);

        physics[i].isGrounded = grounded;
        ++i;
    }
}

void PhysicsSystem::collideCharacterwithWorld(CharacterPhysics * physics,
                                              Transform * transforms,
                                              size_t n,
                                              uint32_t characterIndex,
                                              prt::hash_map<uint16_t, size_t> const & tagToCharacter,
                                              bool & grounded) {
    // unpack variables
    glm::vec3 & position = transforms[characterIndex].position;
    glm::vec3 & velocity = physics[characterIndex].velocity;
    glm::vec3 const & radii = m_ellipsoids[physics[characterIndex].colliderTag.index];
    glm::vec3 & groundNormal = physics[characterIndex].groundNormal;
    ColliderTag const tag = { physics[characterIndex].colliderTag.index, ColliderType::COLLIDER_TYPE_ELLIPSOID };

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
            // broad-phase query
            AABB eAABB = { position - radii, position + radii };
            eAABB += { position + velocity - radii, 
                    position + velocity + radii };
            prt::vector<uint16_t> meshColIDs; 
            prt::vector<uint16_t> ellipsoidColIDs; 
            m_aabbData.tree.query(tag, eAABB, meshColIDs, ellipsoidColIDs);

            // collide with meshes
            if (!meshColIDs.empty()) {                
                glm::vec3 ip;
                float t;
                glm::vec3 cn;
                if (collideCharacterWithMeshes(position, velocity, radii, meshColIDs, 
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
            if (collideCharacterWithCharacters(physics, transforms, n, characterIndex, ellipsoidColIDs, tagToCharacter,
                                               ip, t, cn, oci) &&
                t < intersectionTime) {
                collision = true;
                intersectionTime = t;
                intersectionPoint = ip;
                collisionNormal = cn;

                otherCharacterIndex = oci;
            }
        }
        // respond
        collisionResponse(collision, intersectionPoint, collisionNormal, intersectionTime, 
                          physics, transforms, characterIndex, otherCharacterIndex);
        if (collision) { 
            // set grounded
            bool groundCollision = glm::dot(collisionNormal, glm::vec3{0.0f,1.0f,0.0f}) > 0.0f;
            grounded = grounded || groundCollision;

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
                                               prt::vector<ColliderIndex> const & colliderIndices,
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
    for (uint32_t colliderIndex : colliderIndices) {
        nIndices += m_models.meshes[colliderIndex].numIndices;
        
    }
    // fill vector with polygons in ellipsoid space
    prt::vector<Polygon> polygons;
    polygons.resize(nIndices / 3);
    glm::vec3* pCurr = &polygons[0].a;
    for (uint32_t colliderIndex : colliderIndices) {
        MeshCollider & meshCollider = m_models.meshes[colliderIndex];

        Geometry & geometry = m_models.geometries[meshCollider.modelIndex];
        
        size_t endIndex = meshCollider.startIndex + meshCollider.numIndices;
        for (size_t i = meshCollider.startIndex; i < endIndex; ++i) {
            *pCurr = geometry.cache[i] / ellipsoidRadii;
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
    return /*0.0f <= intersectionTime &&*/ intersectionTime <= 1.0f;
}

bool PhysicsSystem::collideCharacterWithCharacters(CharacterPhysics * physics,
                                                   Transform * transforms,
                                                   size_t /*n*/,
                                                   uint32_t characterIndex,
                                                   prt::vector<uint16_t> const & colliderIDs,
                                                   prt::hash_map<uint16_t, size_t> const & tagToCharacter,
                                                   glm::vec3 & intersectionPoint,
                                                   float & intersectionTime,
                                                   glm::vec3 & collisionNormal,
                                                   uint32_t & otherCharacterIndex) {
    bool collision = false;
    // TODO: implement a breadth first so that we can avoid
    // O(n^2) when collision is checked for all characters
    intersectionTime = std::numeric_limits<float>::max();
    for (uint16_t id : colliderIDs) {
        auto i = tagToCharacter[id];
        if (i == characterIndex) continue;
        float t;
        glm::vec3 ip;
        glm::vec3 cn;

        glm::vec3 const & velocity = physics[characterIndex].velocity;
        glm::vec3 const & otherVelocity = physics[i].velocity;

        if (collideEllipsoids(m_ellipsoids[physics[characterIndex].colliderTag.index],
                              transforms[characterIndex].position,
                              velocity,
                              m_ellipsoids[physics[i].colliderTag.index],
                              transforms[i].position,
                              otherVelocity,
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
    glm::mat3 D1 = glm::diagonal3x3(1.0f / (ellipsoid1 * ellipsoid1));

    // Compute K2.
    glm::mat3 D0NegHalf = glm::diagonal3x3(ellipsoid0);

    glm::mat3 D0Half = glm::diagonal3x3(1.0f / ellipsoid0);

    glm::vec3 K2 = (K1 - K0) * glm::transpose(R0) * D0Half;
    // Compute M2.
    glm::mat3 R1TR0D0NegHalf = R0 * D0NegHalf * glm::transpose(R1); 
    glm::mat3 M2 = D1 * R1TR0D0NegHalf * glm::transpose(R1TR0D0NegHalf);
    // Factor M2 = R*D*R^T. 
    glm::mat3 Q = math_util::diagonalizer(M2);
    glm::mat3 D = glm::transpose(Q) * M2 * Q;
    glm::mat3 R = glm::transpose(Q);
    // Compute K. 
    glm::vec3 K = K2 * glm::transpose(R);
    // Compute W.
    glm::vec3 W = D0Half * (velocity1 - velocity0) * R0 * R;
    // relative velocity is stationary
    if (glm::length(W) == 0.0f) return false;
    // Transformed ellipsoid0 is Z^T*Z = 1 and transformed ellipsoid1 is 
    // (Z-K)^T*D*(Z-K) = 0.

    // Compute the initial closest point. 
    glm::vec3 P0;
    if (computeClosestPointEllipsoids(D, K, P0) >= 0.0f) {
        // The ellipsoid contains the origin, so the ellipsoids were not 
        // separated.
        return false;
    }
    
    // float dist0 = glm::dot(P0, P0) - 1.0f;
    float dist0 = glm::dot(P0, P0) / glm::length(W) - 1.0f; // tolerance for some overlap
    if (dist0 < 0.0f) {
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
        // if (hder > 0.0f) { // this would result in division by zero
        if (hder >= 0.0f) {
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
        return std::numeric_limits<float>::max();
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
        float fder = -2.0f*(d0 * d0k0k0/tmp0cub + d1 * d1k1k1 / tmp1cub
                            + d2 * d2k2k2 / tmp2cub);
        // Compute the next iterate sNext = s - f(s)/f’(s).
        if (fder == 0.0f) return std::numeric_limits<float>::max();;
        s -= f/fder;
    }
    closestPoint[0] = d0k0 * s / ( d0 * s - 1.0f); 
    closestPoint[1] = d1k1 * s / ( d1 * s - 1.0f); 
    closestPoint[2] = d2k2 * s / ( d2 * s - 1.0f); 
    return s;
}

void PhysicsSystem::collisionResponse(bool collision,
                                      glm::vec3 const & /*intersectionPoint*/,
                                      glm::vec3 const & collisionNormal,
                                      float const intersectionTime,
                                      CharacterPhysics * physics,
                                      Transform * transforms,
                                      uint32_t characterIndex,
                                      uint32_t otherCharacterIndex) {
    CharacterPhysics & cPhysics = physics[characterIndex];
    glm::vec3 & position = transforms[characterIndex].position;
    
    static constexpr float verySmallDistance = 0.005f;
    if (collision) {
        position += intersectionTime * cPhysics.velocity;
        glm::vec3 slideNormal = collisionNormal;
        // remaining project velocity onto slide normal
        cPhysics.velocity = cPhysics.velocity - ((glm::dot(cPhysics.velocity, slideNormal)) * slideNormal);

        // by pushing out the character along the slide normal we gain
        // tolerance against small numerical errors
        position += verySmallDistance * slideNormal;

        if (otherCharacterIndex != uint32_t(-1)) {
            CharacterPhysics & otherPhysics = physics[otherCharacterIndex];
            glm::vec3 & otherPosition = transforms[otherCharacterIndex].position;

            otherPosition += intersectionTime * otherPhysics.velocity;
            slideNormal = -collisionNormal;
            // project remaining velocity onto slide normal
            otherPhysics.velocity =  otherPhysics.velocity - ((glm::dot( otherPhysics.velocity, slideNormal)) * slideNormal);

            // by pushing out the character along the slide normal we gain
            // tolerance against small numerical errors
            otherPosition += verySmallDistance * slideNormal;
        }
    } else {
        position += cPhysics.velocity;
    }
}
