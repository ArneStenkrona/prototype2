#include "character.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"


CharacterSystem::CharacterSystem(Input & input, 
                                 Camera & camera, 
                                 PhysicsSystem & physicsSystem,
                                 AssetManager & assetManager)
    : m_camera{camera},
      m_playerController{input, camera}, 
      m_physicsSystem{physicsSystem},
      m_assetManager{assetManager} {
    initPlayer();
}

void CharacterSystem::initPlayer() {
    char const *monkeyStr = "duck/duck.dae";
    uint32_t modelID;
    m_assetManager.loadModels(&monkeyStr, 1, &modelID, true);
    addCharacter(modelID);
    addCharacter(modelID);
    // addCharacter(modelID);
    // addCharacter(modelID);
}

void CharacterSystem::addCharacter(uint32_t modelID) {
    size_t index = m_characters.size;
    assert(index < m_characters.maxSize && "Character amount exceeded!");
    ++m_characters.size;

    m_characters.modelIDs[index] = modelID;

    m_characters.transforms[index].scale = {0.6f, 0.6f, 0.6f};
    m_characters.transforms[index].position = {0.6f, -20.0f, 0.6f};

    m_characters.physics[index].colliderID = m_physicsSystem.addEllipsoidCollider({0.5f, 1.0f, 0.5f}, index);

    m_characters.animationClips[index].idle = m_assetManager.getModelManager().getAnimationIndex(modelID, "idle");
    m_characters.animationClips[index].walk = m_assetManager.getModelManager().getAnimationIndex(modelID, "walk");
    m_characters.animationClips[index].run = m_assetManager.getModelManager().getAnimationIndex(modelID, "run");
}

void CharacterSystem::updateCharacters(float deltaTime) {
    // player input
    m_playerController.updateInput(m_characters.input[PLAYER_ID]);

    // JUST FOR FUN, WILL REMOVE LATER
    glm::vec3 dir = m_characters.transforms[0].position - m_characters.transforms[1].position;
    m_characters.input[1].move = glm::vec2(dir.x,dir.z);
    if (glm::length2(m_characters.input[1].move) > 0.0f) m_characters.input[1].move = glm::normalize(m_characters.input[1].move);

    for (size_t index = 0; index < m_characters.size; ++index) {
        updateCharacter(index, deltaTime);
    }
}

void CharacterSystem::updatePhysics() {
    m_physicsSystem.updateCharacterPhysics(m_characters.physics,
                                           m_characters.transforms,
                                           m_characters.size);
}

void CharacterSystem::updateCamera() {
    glm::vec3 hit;
    glm::vec3 corners[4];
    float dist = 5.0f;
    m_camera.getCameraCorners(corners[0], corners[1], corners[2], corners[3]);

    auto const & transform = m_characters.transforms[PLAYER_ID];
    for (size_t i = 0; i < 4; ++i) {
        glm::vec3 dir = glm::normalize(corners[i] - transform.position);
        if (m_physicsSystem.raycast(transform.position, dir, 
                                    5.0f, hit)) {
            dist = std::min(dist, glm::distance(transform.position, hit));

        }
    }
    m_camera.setTargetDistance(dist);
    m_camera.setTarget(transform.position);
}

void CharacterSystem::updateCharacter(size_t index, float deltaTime) {
    // declare aliases to shorten code
    auto const & input = m_characters.input[index];
    auto & transform = m_characters.transforms[index];
    auto & physics = m_characters.physics[index];
    auto & animation = m_characters.animation[index];
    auto const & clips = m_characters.animationClips[index];
    
    glm::vec3 targetVelocity{0.0f};

    // if player performed any movement input
    if (glm::length2(input.move) > 0.0f) {
        glm::vec3 const origin{0.0f, 0.0f, 0.0f}; 
        glm::vec3 const lookDir{input.move.x, 0.0f, input.move.y}; 
        glm::vec3 const up{0.0f, 1.0f, 0.0f}; 
        glm::vec3 const altUp{0.0f, 0.0f, 1.0f}; 
        // rotate character
        transform.rotation = math_util::safeQuatLookAt(origin, lookDir, up, altUp);
        // compute movement plane
        glm::vec3 const moveNormal = physics.isGrounded ? physics.groundNormal : up;
        // project look direction onto normal of movement plane
        glm::vec3 const moveDir = glm::normalize(glm::cross(moveNormal, glm::cross(lookDir, moveNormal)));

        if (input.run) {
            targetVelocity = 0.2f * moveDir;
        } else {
            targetVelocity = 0.05f * moveDir;
        }
    }
    
    // TODO: make gravity and jump the responsibility of
    // the physics system?
    float gAcc = m_physicsSystem.getGravity() * deltaTime;
    physics.gravityVelocity += glm::vec3{0.0f, -1.0f, 0.0f} * gAcc;

    // jump
    if (input.jump && physics.isGrounded) {
        physics.gravityVelocity += 0.5f * glm::vec3{0.0f, 1.0f, 0.0f};
    }

    physics.velocity = glm::lerp(physics.velocity, targetVelocity, 10.0f * deltaTime);
    
    // animation
    float animationDelta = 0.0f;
    float const vmag = glm::length(physics.velocity);
    if (vmag > 0.05f) {
        animation.clipA = clips.walk;
        animation.clipB = clips.run;
        animation.blendFactor = (vmag - 0.05f) / 0.15f;

        animationDelta = math_util::lerp(0.75f, 1.5f, animation.blendFactor) * deltaTime;
    } else {
        animation.clipA = clips.idle;
        animation.clipB = clips.walk;
        animation.blendFactor = vmag / 0.05f;
        animationDelta = math_util::lerp(0.6f, 0.75f, animation.blendFactor) * deltaTime;
    }

    animation.blendFactor = glm::clamp(animation.blendFactor, 0.0f, 1.0f);
    animation.time += animationDelta;
}

void CharacterSystem::sampleAnimation(prt::vector<glm::mat4> & bones) {
    m_assetManager.getModelManager().getSampledBlendedAnimation(m_characters.modelIDs,
                                                                m_characters.animation,
                                                                bones,
                                                                m_characters.size);
}

void CharacterSystem::getTransformMatrices(prt::vector<glm::mat4>& transformMatrices) const {
    transformMatrices.resize(m_characters.size);
    for (size_t i = 0; i < m_characters.size; ++i) {
        transformMatrices[i] = m_characters.transforms[i].transformMatrix();
    } 
}
