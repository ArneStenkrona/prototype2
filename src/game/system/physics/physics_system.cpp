#include "physics_system.h"

#include "src/game/scene/scene.h"

#include "src/util/physics_util.h"
#include "src/util/math_util.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/string_cast.hpp>

#include <dirent.h>

PhysicsSystem::PhysicsSystem() 
    : m_collisionSystem{} {}

void PhysicsSystem::newFrame() {
    m_collisionSystem.newFrame();
}

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

    m_aabbData.capsuleAABBs.push_back(capsule.getAABB(glm::mat4{1.0f}));

    ColliderTag tag = { uint16_t(id), ColliderShape::COLLIDER_SHAPE_CAPSULE, ColliderType::COLLIDER_TYPE_COLLIDE };
    m_aabbData.tree.insert(&tag, &m_aabbData.capsuleAABBs[id], 1, &m_aabbData.capsuleIndices[id]);

    return tag;
}

ColliderTag PhysicsSystem::addModelCollider(Model const & model, Transform const & transform) {
    ColliderTag tag;
    tag.shape = COLLIDER_SHAPE_MODEL;


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
        tags.push_back({ColliderIndex(i), ColliderShape::COLLIDER_SHAPE_MESH, ColliderType::COLLIDER_TYPE_COLLIDE });
    }
    m_aabbData.tree.insert(tags.data(), m_aabbData.meshAABBs.data() + prevSize, numMesh, m_aabbData.meshIndices.data() + prevSize);


    return tag;
}

void PhysicsSystem::removeCollider(ColliderTag const & tag) {
    switch (tag.shape) {
        case COLLIDER_SHAPE_MODEL:
            removeModelCollider(tag.index);
            break;
        case COLLIDER_SHAPE_CAPSULE:
            assert(false && "Not yet implemeneted!");
            break;
        default:
            assert(false && "This collider shape can not be removed!");

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
    assert(tag.shape == COLLIDER_SHAPE_CAPSULE);
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
        assert(tags[i].shape == COLLIDER_SHAPE_MODEL);
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
        if (tag.shape != COLLIDER_SHAPE_MESH) {
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


void PhysicsSystem::updateCharacters(float deltaTime,
                                     Scene & scene,
                                     Transform * transforms) {
    // unpack variables
    CharacterSystem & characterSystem = scene.getCharacterSystem();
    size_t n = characterSystem.getNumberOfCharacters();
    // CharacterPhysics * physics = characterSystem.getCharacterPhysics();
    // CharacterAttributeInfo * attributeInfos = characterSystem.getCharacterAttribueInfos();

    // update character AABBs
    prt::hash_map<uint16_t, size_t> tagToCharacter;
    prt::vector<glm::vec3> prevVelocities;
    prevVelocities.resize(n);

    size_t i = 0;    
    while (i < n) {
        Character & character = characterSystem.getCharacter(i);

        CapsuleCollider const & capsule = m_capsules[character.physics.colliderTag.index];
        AABB & eAABB = m_aabbData.capsuleAABBs[character.physics.colliderTag.index];

        Transform & transform = transforms[i];
        glm::mat4 tform = glm::translate(glm::mat4(1.0f), transform.position) * glm::toMat4(glm::normalize(transform.rotation));
        eAABB = capsule.getAABB(tform);

        float gravityFactor = m_gravity;
        glm::mat4 velTform = glm::translate(tform, character.physics.velocity + glm::vec3{0.0f, -1.0f, 0.0f} * gravityFactor * deltaTime);
        
        eAABB += capsule.getAABB(velTform);

        tagToCharacter.insert(character.physics.colliderTag.index, i);

        prevVelocities[i] = character.physics.velocity;

        if (character.physics.isGrounded) {
            character.physics.velocity += (-0.05f * gravityFactor * deltaTime) * character.physics.groundNormal;
        } 
        character.physics.velocity.x += character.physics.movementVector.x;
        character.physics.velocity.z += character.physics.movementVector.z;

        character.physics.isGrounded = false;

        ++i;
    }
    m_aabbData.tree.update(m_aabbData.capsuleIndices.data(), m_aabbData.capsuleAABBs.data(), m_capsules.size());
    
    // collide
    i = 0;
    while (i < n) {
        // movement
        collideCharacterWithWorld(scene, transforms, i, tagToCharacter);
        ++i;
    }
    i = 0;
    while (i < n) {
        Character & character = characterSystem.getCharacter(i);

        float gravityFactor = m_gravity;

        character.physics.velocity = prevVelocities[i];
        // TODO: formalize friction
        // friction
        float frictionRatio = 1 / (1 + (deltaTime * 10.0f));
        character.physics.velocity.x = character.physics.velocity.x * frictionRatio;
        character.physics.velocity.z = character.physics.velocity.z * frictionRatio;
        
        if (character.physics.isGrounded) {
            character.physics.velocity.y = glm::max(0.0f * gravityFactor * deltaTime, character.physics.velocity.y);
        } else {
            character.physics.velocity.y += -1.0f * gravityFactor * deltaTime;
        }
        ++i;
    }
}

// void PhysicsSystem::updateTriggers(float deltaTime,
//                                    ColliderTag const * triggers,
//                                    Transform * transforms,
//                                    size_t n) {
//     // update triggers
//     for (int i = 0; i < n; ++i) {

//     }
// }

void PhysicsSystem::collideCharacterWithWorld(Scene & scene,
                                              Transform * transforms,
                                              uint32_t characterIndex,
                                              prt::hash_map<uint16_t, size_t> const & tagToCharacter) {
    // unpack variables
    CharacterSystem & characterSystem = scene.getCharacterSystem();
    Character & character = characterSystem.getCharacter(characterIndex);
    CharacterPhysics & physics = character.physics;

    ColliderTag const & tag = physics.colliderTag;
    AABB & eAABB = m_aabbData.capsuleAABBs[tag.index];
    CapsuleCollider const & capsule = m_capsules[tag.index];
    Transform & transform = transforms[characterIndex];

    glm::mat4 prevTform = glm::translate(glm::mat4(1.0f), transform.position) * glm::toMat4(glm::normalize(transform.rotation));

    static constexpr unsigned int nTimeSteps = 4;
    unsigned int stepsLeft = nTimeSteps;
    while (stepsLeft != 0) {
        --stepsLeft;
        transform.position += physics.velocity / float(nTimeSteps);

        glm::mat4 tform = glm::translate(glm::mat4(1.0f), transform.position) * glm::toMat4(glm::normalize(transform.rotation));

        // broad-phase query
        eAABB = capsule.getAABB(prevTform);
        eAABB += capsule.getAABB(tform);

        prevTform = tform;
        
        CollisionPackage package{};
        package.tag = tag;
        package.transform = &transforms[characterIndex];
        package.character = &character;

        prt::vector<uint16_t> meshColIDs; 
        prt::vector<uint16_t> capsuleColIDs; 
        m_aabbData.tree.query(tag, eAABB, meshColIDs, capsuleColIDs, COLLIDER_TYPE_COLLIDE);

        for (uint32_t colID : capsuleColIDs) {
            size_t otherCharacterIndex = tagToCharacter[colID];

            Character & otherCharacter = characterSystem.getCharacter(otherCharacterIndex);
            CharacterPhysics & otherPhysics = otherCharacter.physics;

            ColliderTag const & otherTag = otherPhysics.colliderTag;
            CapsuleCollider const & otherCapsule = m_capsules[otherTag.index];
            Transform & otherTransform = transforms[otherCharacterIndex];

            CollisionPackage packageOther{};
            packageOther.tag = otherTag;
            packageOther.transform = &otherTransform;
            packageOther.character = &otherCharacter;

            m_collisionSystem.collideCapsuleCapsule(scene,
                                                    package,
                                                    capsule,
                                                    packageOther,
                                                    otherCapsule);
        }
        
        if (meshColIDs.empty()) {
            break;
        }

        // create aggregate mesh collider
        AggregateMeshCollider aggregateCollider;
        aggregateCollider.tagOffsets.resize(meshColIDs.size());

        // construct all polygons
        // count all indices
        size_t nIndices = 0;
        for (unsigned int i = 0; i < meshColIDs.size(); ++i) {
            uint32_t colID = meshColIDs[i];

            aggregateCollider.tagOffsets[i].offset = nIndices / 3;

            aggregateCollider.tagOffsets[i].tag.index = colID;
            aggregateCollider.tagOffsets[i].tag.type = COLLIDER_TYPE_COLLIDE;
            aggregateCollider.tagOffsets[i].tag.shape = COLLIDER_SHAPE_MESH;
            
            nIndices += m_models.meshes[colID].numIndices;
        }
        // fill vector with polygons in ellipsoid space


        // prt::vector<Polygon> polygons;
        aggregateCollider.polygons.resize(nIndices / 3);
        glm::vec3* pCurr = &aggregateCollider.polygons[0].a;
        for (uint32_t colID : meshColIDs) {
            MeshCollider & meshCollider = m_models.meshes[colID];

            Geometry & geometry = m_models.geometries[meshCollider.modelIndex];
            
            size_t endIndex = meshCollider.startIndex + meshCollider.numIndices;
            for (size_t i = meshCollider.startIndex; i < endIndex; ++i) {
                *pCurr = geometry.cache[i];
                ++pCurr;
            }
        }

        m_collisionSystem.collideCapsuleMesh(scene,
                                             package,
                                             capsule,
                                             aggregateCollider);
        
    }
    transform.position += float(stepsLeft) * physics.velocity / float(nTimeSteps);
}
