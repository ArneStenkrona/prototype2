#include "character.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"

#include "glm/gtx/string_cast.hpp"


CharacterSystem::CharacterSystem(Input & input, 
                                 Camera & camera, 
                                 PhysicsSystem & physicsSystem,
                                 AssetManager & assetManager)
    : m_camera{camera},
      m_playerController{input, camera}, 
      m_physicsSystem{physicsSystem},
      m_assetManager{assetManager} {
}

void CharacterSystem::updateCharacters(float deltaTime) {
    // player input
    m_playerController.updateInput(m_characters.input[PLAYER_ID]);

    // JUST FOR FUN, WILL REMOVE LATER
    if (m_characters.size > 1) {
        glm::vec3 dir = m_characters.transforms[0].position - m_characters.transforms[1].position;
        m_characters.input[1].move = glm::vec2(dir.x,dir.z);
        if (glm::length2(m_characters.input[1].move) > 0.0f) m_characters.input[1].move = glm::normalize(m_characters.input[1].move);
    }

    for (size_t index = 0; index < m_characters.size; ++index) {
        updateCharacter(index, deltaTime);
    }
}

void CharacterSystem::updatePhysics(float deltaTime) {
    m_physicsSystem.updateCharacterPhysics(deltaTime,
                                           m_characters.physics,
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
    
    glm::vec3 targetMovement{0.0f};

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
            targetMovement = 0.08f * moveDir;
        } else {
            targetMovement = 0.025f * moveDir;
        }
    }
    float animationDelta = 0.0f;
    
    physics.movementVector = glm::lerp(physics.movementVector, targetMovement, 10.0f * deltaTime);
    if (physics.isGrounded) {
        physics.isJumping = false;
        physics.airTime = 0.0f;
        // jump
        if (input.jump) {
            physics.velocity += 0.25f * glm::vec3{0.0f, 1.0f, 0.0f};
            physics.isJumping = true;

            animation.clipA = clips.jump;
            animation.blendFactor = 0;
            animation.time = 0;
            animationDelta = 1.0 * deltaTime;
        }
    } else {
        physics.airTime += deltaTime;
    }
    physics.velocity = physics.movementVector + glm::vec3{ 0.0f, physics.velocity.y, 0.0f };
    
    // animation
    float const vmag = glm::length(physics.movementVector);

    /*if (!physics.isGrounded && physics.airTime > 0.1f && physics.velocity.y < 0.0f) {
        animationDelta = 1.0 * deltaTime;
        animation.clipA = clips.jump;
        animation.clipB = clips.fall;
        animation.blendFactor = 1;  

    } else if (physics.isJumping) {
        animationDelta = 1.0 * deltaTime;
        animation.clipA = clips.jump;
        animation.clipB = clips.fall;
        animation.blendFactor = glm::min(1.0f, animation.blendFactor + deltaTime);
        
    } else */if (vmag > 0.025f) {
        animation.clipA = clips.walk;
        animation.clipB = clips.run;
        animation.blendFactor = (vmag - 0.025f) / 0.055f;

        animationDelta = math_util::lerp(0.75f, 1.5f, animation.blendFactor) * deltaTime;
    } else {
        animation.clipA = clips.idle;
        animation.clipB = clips.walk;
        animation.blendFactor = vmag / 0.025f;
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
