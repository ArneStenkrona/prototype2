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

ColliderTag PhysicsSystem::addCapsuleCollider(float height,
                                              float radius,
                                              glm::vec3 const & offset) {
    assert(m_capsules.size() < std::numeric_limits<uint16_t>::max() && "Too many capsule colliders!");
    uint16_t id = m_capsules.size();

    m_capsules.push_back({});
    CapsuleCollider & capsule = m_capsules.back();
    capsule.height = height;
    capsule.radius = radius;
    capsule.offset = offset;

    m_aabbData.capsuleIndices.push_back({});

    m_aabbData.capsuleAABBs.push_back(capsule.getAABB(glm::vec3{0.0f}));

    ColliderTag tag = { uint16_t(id), ColliderType::COLLIDER_TYPE_CAPSULE };
    m_aabbData.tree.insert(&tag, &m_aabbData.capsuleAABBs[id], 1, &m_aabbData.capsuleIndices[id]);

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
        case COLLIDER_TYPE_CAPSULE:
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

void PhysicsSystem::updateCapsuleCollider(ColliderTag const & tag, 
                                 float height, 
                                 float radius,
                                 glm::vec3 const & offset) {
    assert(tag.type == COLLIDER_TYPE_CAPSULE);
    m_capsules[tag.index].height = height;
    m_capsules[tag.index].radius = radius;
    m_capsules[tag.index].offset = offset;
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
        CapsuleCollider const & capsule = m_capsules[physics[i].colliderTag.index];
        AABB & eAABB = m_aabbData.capsuleAABBs[physics[i].colliderTag.index];

        eAABB = capsule.getAABB(transforms[i].position);

        float gravityFactor = m_gravity;
        
        eAABB += capsule.getAABB(transforms[i].position + physics[i].velocity + glm::vec3{0.0f, -1.0f, 0.0f} * gravityFactor * deltaTime);

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
    m_aabbData.tree.update(m_aabbData.capsuleIndices.data(), m_aabbData.capsuleAABBs.data(), m_capsules.size());
    
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
                                              size_t /*n*/,
                                              uint32_t characterIndex,
                                              prt::hash_map<uint16_t, size_t> const & tagToCharacter) {
    // unpack variables
    CapsuleCollider const & capsule = m_capsules[physics[characterIndex].colliderTag.index];
    // glm::vec3 position = transforms[characterIndex].position + offset;
    // glm::vec3 & velocity = physics[characterIndex].velocity;
    ColliderTag const tag = { physics[characterIndex].colliderTag.index, ColliderType::COLLIDER_TYPE_CAPSULE };

    glm::vec3 prevPosition = transforms[characterIndex].position;

    static constexpr unsigned int nTimeSteps = 4;
    unsigned int stepsLeft = nTimeSteps;
    while (stepsLeft != 0) {
        --stepsLeft;
        transforms[characterIndex].position += physics[characterIndex].velocity / float(nTimeSteps);

        glm::vec3 position = transforms[characterIndex].position; 
        // broad-phase query
        AABB eAABB = capsule.getAABB(prevPosition);
        eAABB += capsule.getAABB(position);

        prt::vector<uint16_t> meshColIDs; 
        prt::vector<uint16_t> capsuleColIDs; 
        m_aabbData.tree.query(tag, eAABB, meshColIDs, capsuleColIDs);

        if (meshColIDs.empty()) {
            break;
        }

        // construct all polygons
        // count all indices
        size_t nIndices = 0;
        for (uint32_t colID : meshColIDs) {
            nIndices += m_models.meshes[colID].numIndices;
            
        }
        // fill vector with polygons in ellipsoid space
        prt::vector<Polygon> polygons;
        polygons.resize(nIndices / 3);
        glm::vec3* pCurr = &polygons[0].a;
        for (uint32_t colID : meshColIDs) {
            MeshCollider & meshCollider = m_models.meshes[colID];

            Geometry & geometry = m_models.geometries[meshCollider.modelIndex];
            
            size_t endIndex = meshCollider.startIndex + meshCollider.numIndices;
            for (size_t i = meshCollider.startIndex; i < endIndex; ++i) {
                *pCurr = geometry.cache[i];
                ++pCurr;
            }
        }
        prevPosition = position;
        
        CollisionPackage package{};
        package.type = COLLISION_TYPE_RESPOND;
        package.transform = &transforms[characterIndex];
        package.physics = &physics[characterIndex];

        collideCapsuleMesh(package,
                           capsule,
                           polygons.data(),
                           polygons.size());
        
        for (uint32_t colID : capsuleColIDs) {
            CapsuleCollider const & capsuleOther = m_capsules[physics[colID].colliderTag.index];
            
            size_t otherCharacterIndex = tagToCharacter[colID];

            CollisionPackage packageOther{};
            packageOther.type = COLLISION_TYPE_RESPOND;
            packageOther.transform = &transforms[otherCharacterIndex];
            packageOther.physics = &physics[otherCharacterIndex];

            collideCapsuleCapsule(package,
                                  capsule,
                                  packageOther,
                                  capsuleOther);
        }
    }
    transforms[characterIndex].position += float(stepsLeft) * physics[characterIndex].velocity / float(nTimeSteps);
}
