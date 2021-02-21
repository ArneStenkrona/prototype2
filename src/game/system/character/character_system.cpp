#include "character_system.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"

#include "glm/gtx/string_cast.hpp"

#include "src/game/scene/scene.h"

#include <cstdlib>

CharacterSystem::CharacterSystem(Scene * scene, 
                                 PhysicsSystem & physicsSystem,
                                 AnimationSystem & animationSystem)
    : m_scene{scene},
      m_physicsSystem{physicsSystem},
      m_animationSystem{animationSystem},
      m_playerController{scene->getInput(), scene->getCamera()} {
}

void CharacterSystem::updateCharacters(float deltaTime) {
    // player input
    m_playerController.updateInput(m_characters.input[PLAYER_ID]);

    // JUST FOR FUN, WILL REMOVE LATER
    if (m_characters.size > 1) {
        glm::vec3 dir = m_scene->getTransform(m_characters.entityIDs[0]).position - m_scene->getTransform(m_characters.entityIDs[1]).position;
        m_characters.input[1].move = glm::vec2(dir.x,dir.z);
        if (glm::length2(m_characters.input[1].move) > 0.0f) m_characters.input[1].move = glm::normalize(m_characters.input[1].move);
    }

    for (CharacterID index = 0; index < m_characters.size; ++index) {
        updateCharacter(index, deltaTime);
    }
}

CharacterID CharacterSystem::addCharacter(EntityID entityID, ColliderTag tag, CharacterAnimationClips clips, float animationSpeed) { 
    assert(m_characters.size < m_characters.maxSize && "Character amount exceeded!");

    m_characters.entityIDs[m_characters.size] = entityID;
    m_characters.physics[m_characters.size].colliderTag = tag;
    m_characters.animationClips[m_characters.size] = clips;
    m_characters.animationSpeeds[m_characters.size] = animationSpeed;

    ++m_characters.size; 
    return m_characters.size - 1; 
}
                                                  
void CharacterSystem::updatePhysics(float deltaTime) {
    prt::vector<Transform> transforms;
    transforms.resize(m_characters.size);

    for (CharacterID i = 0; i < m_characters.size; ++i) {
        transforms[i] = m_scene->getTransform(m_characters.entityIDs[i]);
    }

    m_physicsSystem.updateCharacterPhysics(deltaTime,
                                           m_characters.physics,
                                           transforms.data(),
                                           m_characters.size);

    for (CharacterID i = 0; i < m_characters.size; ++i) {
        m_scene->getTransform(m_characters.entityIDs[i]) = transforms[i];
    }
}

void CharacterSystem::updateCharacter(CharacterID characterID, float deltaTime) {
    // declare aliases to shorten code
    auto const & input = m_characters.input[characterID];
    auto & transform = m_scene->getTransform(m_characters.entityIDs[characterID]);
    auto & physics = m_characters.physics[characterID];
    auto & animation = m_animationSystem.getAnimationBlend(m_characters.entityIDs[characterID]);
    auto const & clips = m_characters.animationClips[characterID];
    float animationSpeed = m_characters.animationSpeeds[characterID];
    
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

        animationDelta = math_util::lerp(0.75f, 1.5f, animation.blendFactor) * deltaTime * animationSpeed;
    } else {
        animation.clipA = clips.idle;
        animation.clipB = clips.walk;
        animation.blendFactor = vmag / 0.025f;
        animationDelta = math_util::lerp(0.6f, 0.75f, animation.blendFactor) * deltaTime * animationSpeed;
    }

    animation.blendFactor = glm::clamp(animation.blendFactor, 0.0f, 1.0f);
    animation.time += animationDelta;
}
