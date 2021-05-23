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

ColliderTag PhysicsSystem::addEllipsoidCollider(glm::vec3 const & radii,
                                                glm::vec3 const & offset) {
    assert(m_ellipsoids.size() < std::numeric_limits<uint16_t>::max() && "Too many ellipsoid colliders!");
    uint16_t id = m_ellipsoids.size();

    m_ellipsoids.push_back({});
    EllipsoidCollider & ellipsoid = m_ellipsoids.back();
    ellipsoid.radii = radii;
    ellipsoid.offset = offset;

    m_aabbData.ellipsoidIndices.push_back({});

    m_aabbData.ellipsoidAABBs.push_back(ellipsoid.getAABB(glm::vec3{0.0f}));

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

void PhysicsSystem::updateEllipsoidCollider(ColliderTag const & tag, 
                                            glm::vec3 const & radii,
                                            glm::vec3 const & offset) {
    assert(tag.type == COLLIDER_TYPE_ELLIPSOID);
    m_ellipsoids[tag.index].radii = radii;
    m_ellipsoids[tag.index].offset = offset;
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
        if (tag.type != COLLIDER_TYPE_MESH) {
            continue;
        }
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
    
    // update character AABBs
    prt::hash_map<uint16_t, size_t> tagToCharacter;
    prt::vector<glm::vec3> prevVelocities;
    prevVelocities.resize(n);

    size_t i = 0;    
    while (i < n) {
        EllipsoidCollider const & ellipsoid = m_ellipsoids[physics[i].colliderTag.index];
        AABB & eAABB = m_aabbData.ellipsoidAABBs[physics[i].colliderTag.index];

        eAABB = ellipsoid.getAABB(transforms[i].position);

        float gravityFactor = m_gravity;

        eAABB += { transforms[i].position + ellipsoid.offset + physics[i].velocity + glm::vec3{0.0f, -1.0f, 0.0f} * gravityFactor * deltaTime - ellipsoid.radii, 
                   transforms[i].position + ellipsoid.offset + physics[i].velocity + glm::vec3{0.0f, -1.0f, 0.0f} * gravityFactor * deltaTime + ellipsoid.radii };

        tagToCharacter.insert(physics[i].colliderTag.index, i);

        prevVelocities[i] = physics[i].velocity;

        if (physics[i].isGrounded) {
            physics[i].velocity += (-0.05f * gravityFactor * deltaTime) * physics[i].groundNormal;
        } 
        physics[i].velocity.x += physics[i].movementVector.x;
        physics[i].velocity.z += physics[i].movementVector.z;

        physics[i].isGrounded = false;

        ++i;
    }
    m_aabbData.tree.update(m_aabbData.ellipsoidIndices.data(), m_aabbData.ellipsoidAABBs.data(), m_ellipsoids.size());
    
    // collide
    i = 0;
    while (i < n) {
        // movement
        collideCharacterwithWorld(physics, transforms, n, i, tagToCharacter);
        ++i;
    }
    i = 0;
    while (i < n) {
        float gravityFactor = m_gravity;

        physics[i].velocity = prevVelocities[i];
        // TODO: formalize friction
        // friction
        float frictionRatio = 1 / (1 + (deltaTime * 10.0f));
        physics[i].velocity.x = physics[i].velocity.x * frictionRatio;
        physics[i].velocity.z = physics[i].velocity.z * frictionRatio;
        
        if (physics[i].isGrounded) {
            physics[i].velocity.y = glm::max(0.0f * gravityFactor * deltaTime, physics[i].velocity.y);
        } else {
            physics[i].velocity.y += -1.0f * gravityFactor * deltaTime;
        }
        ++i;
    }
}

void PhysicsSystem::collideCharacterwithWorld(CharacterPhysics * physics,
                                              Transform * transforms,
                                              size_t n,
                                              uint32_t characterIndex,
                                              prt::hash_map<uint16_t, size_t> const & tagToCharacter) {
    // unpack variables
    glm::vec3 const & radii = m_ellipsoids[physics[characterIndex].colliderTag.index].radii;
    glm::vec3 const & offset = m_ellipsoids[physics[characterIndex].colliderTag.index].offset;
    glm::vec3 position = transforms[characterIndex].position + offset;
    glm::vec3 & velocity = physics[characterIndex].velocity;
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
                    intersectionPoint = ip;
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
        if (collision) { 
            // respond
            collisionResponse(intersectionPoint, collisionNormal, intersectionTime, 
                              physics, transforms, characterIndex);

            if (otherCharacterIndex != uint32_t(-1)) {
                collisionResponse(intersectionPoint, -collisionNormal, intersectionTime, 
                                  physics, transforms, otherCharacterIndex);
            }
        } else {
            // no collision, move character and break
            transforms[characterIndex].position += physics[characterIndex].velocity;
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

    // construct all polygons
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

    bool res = collideSphereMesh(sourcePoint, 
                                 velocityVector, 
                                 polygons.data(),
                                 polygons.size(),
                                 intersectionPoint,
                                 intersectionTime,
                                 collisionNormal);

    // convert to original space
    intersectionPoint = intersectionPoint * ellipsoidRadii;
    collisionNormal = glm::normalize(glm::mat3(glm::inverse(glm::transpose(glm::scale(ellipsoidRadii)))) * collisionNormal);
    return res;
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

        EllipsoidCollider const & characterEllipsoid = m_ellipsoids[physics[characterIndex].colliderTag.index];
        EllipsoidCollider const & otherEllipsoid = m_ellipsoids[physics[i].colliderTag.index];

        if (collideEllipsoidEllipsoid(characterEllipsoid.radii,
                                      transforms[characterIndex].position + characterEllipsoid.offset,
                                      velocity,
                                      otherEllipsoid.radii,
                                      transforms[i].position + otherEllipsoid.offset,
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

void PhysicsSystem::collisionResponse(glm::vec3 const & /*intersectionPoint*/,
                                      glm::vec3 const & collisionNormal,
                                      float const intersectionTime,
                                      CharacterPhysics * physics,
                                      Transform * transforms,
                                      uint32_t characterIndex) {
    CharacterPhysics & cPhysics = physics[characterIndex];
    glm::vec3 & position = transforms[characterIndex].position;

    static constexpr float verySmallDistance = 0.001f;
    // if colliding with another object in motion it
    // is possible that the velocity of this object
    // is actually receding from the collision
    // normal
    bool receding = glm::dot(collisionNormal, cPhysics.velocity) >= 0.0f;
    if (receding) {
        position += intersectionTime * cPhysics.velocity;
        cPhysics.velocity = glm::clamp(1.0f - intersectionTime, 0.0f, 1.0f) * cPhysics.velocity;
    } else {
        position += intersectionTime * cPhysics.velocity;
        // remaining project velocity onto collisionNormal
        glm::vec3 remainingVelocity = glm::clamp(1.0f - intersectionTime, 0.0f, 1.0f) * cPhysics.velocity;
        cPhysics.velocity = remainingVelocity - ((glm::dot(remainingVelocity, collisionNormal)) * collisionNormal);
    }

    // by pushing out the character along the collisionNormal we gain
    // tolerance against small numerical errors
    position += verySmallDistance * collisionNormal;

    // set grounded
    bool groundCollision = glm::dot(collisionNormal, glm::vec3{0.0f,1.0f,0.0f}) > 0.7f;
    physics[characterIndex].isGrounded = physics[characterIndex].isGrounded || groundCollision;

    if (groundCollision) {
        physics[characterIndex].groundNormal = collisionNormal;
    }
}
