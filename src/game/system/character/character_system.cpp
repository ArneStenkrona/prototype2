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
    for (int i = 1; i < m_characters.size; ++i) {
        glm::vec3 dir = m_scene->getTransform(m_characters.entityIDs[0]).position - m_scene->getTransform(m_characters.entityIDs[i]).position;
        m_characters.input[i].move = glm::vec2(dir.x,dir.z);
        if (glm::length2(m_characters.input[i].move) > 0.0f) m_characters.input[i].move = glm::normalize(m_characters.input[i].move);
    }

    for (CharacterID index = 0; index < m_characters.size; ++index) {
        updateCharacter(index, deltaTime);
    }
}

CharacterID CharacterSystem::addCharacter(EntityID entityID, ColliderTag tag, float animationSpeed) { 
    assert(m_characters.size < m_characters.maxSize && "Character amount exceeded!");

    CharacterAnimationClips & clips = m_characters.animationClips[m_characters.size];
    clips.idle = m_scene->getModel(entityID).getAnimationIndex("idle");
    clips.walk = m_scene->getModel(entityID).getAnimationIndex("walk");
    clips.run  = m_scene->getModel(entityID).getAnimationIndex("run");
    clips.jump = m_scene->getModel(entityID).getAnimationIndex("jump");
    clips.fall = m_scene->getModel(entityID).getAnimationIndex("fall");
    clips.land = m_scene->getModel(entityID).getAnimationIndex("land");

    clips.idle = clips.idle == -1 ? 0 : clips.idle;
    clips.walk = clips.walk == -1 ? 0 : clips.walk;
    clips.run  = clips.run == -1 ? 0 : clips.run;
    clips.jump = clips.jump == -1 ? 0 : clips.jump;
    clips.fall = clips.fall == -1 ? 0 : clips.fall;
    clips.land = clips.land == -1 ? 0 : clips.land;

    m_characters.entityIDs[m_characters.size] = entityID;
    m_characters.physics[m_characters.size].colliderTag = tag;
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
    auto const & input = m_characters.input[characterID];
    auto & stateInfo = m_characters.stateInfos[characterID];
    auto & physics = m_characters.physics[characterID];
    auto & animation = m_animationSystem.getAnimationBlend(m_characters.entityIDs[characterID]);
    auto const & clips = m_characters.animationClips[characterID];
    float animationSpeed = m_characters.animationSpeeds[characterID];

    if (physics.isGrounded) {
        stateInfo.groundedTimer = 0.15f;
    } else {
        stateInfo.groundedTimer -= deltaTime;
    }
    updateCharacterInput(characterID, deltaTime);

    CharacterState state = stateInfo.state;
    bool stateChange = stateInfo.state != stateInfo.previousState;

    if (stateChange) {
        stateInfo.stateTimer = 0.0f;
        animation.time = 0.0f;

    } else {
        stateInfo.stateTimer += deltaTime;
        animation.time += stateInfo.animationDelta * animationSpeed * deltaTime;
    }

    switch (stateInfo.state) {
        case CHARACTER_STATE_GROUNDED: {
            if (stateInfo.groundedTimer >= 0.0f && input.jump) {
                stateInfo.state = CHARACTER_STATE_JUMPING;
            } else if (stateInfo.groundedTimer < -0.3f) {
                stateInfo.state = CHARACTER_STATE_FALLING;
            }

            float const vmag = glm::length(physics.movementVector);
    
            if (vmag > 0.025f) {
                animation.clipA = clips.walk;
                animation.clipB = clips.run;
                animation.blendFactor = (vmag - 0.025f) / 0.055f;

                stateInfo.animationDelta = math_util::lerp(0.75f, 1.5f, animation.blendFactor);
            } else {
                animation.clipA = clips.idle;
                animation.clipB = clips.walk;
                animation.blendFactor = vmag / 0.025f;
                stateInfo.animationDelta = math_util::lerp(0.6f, 0.75f, animation.blendFactor);
            }
            
            break;
        }
        case CHARACTER_STATE_JUMPING: {
            animation.clipA = clips.jump;
            animation.clipB = clips.fall;
            animation.blendFactor= math_util::lerp(0.0f, 1.0f, (animation.time - 0.3f) / 0.2f);

            stateInfo.animationDelta = 1.0f;

            if (stateChange) {
                stateInfo.hasJumped = false;
                stateInfo.groundedTimer = -0.1f;
            }

            if (stateInfo.stateTimer > 0.05f &&
                !stateInfo.hasJumped) {
                stateInfo.hasJumped = true;
                physics.velocity.y = 0.35f;
            }

            if (animation.time >= 0.5f) {
                stateInfo.state = CHARACTER_STATE_FALLING;
            }
            
            break;
        }
        case CHARACTER_STATE_FALLING: {
            animation.clipA = clips.fall;
            animation.clipB = clips.fall;
            animation.blendFactor = 0.0f;

            stateInfo.animationDelta = 1.0f;

            if (physics.isGrounded) {
                stateInfo.state = CHARACTER_STATE_LANDING;
            }
            break;
        }
        case CHARACTER_STATE_LANDING: {
            animation.clipA = clips.land;
            animation.clipB = clips.idle;
            animation.blendFactor= math_util::lerp(0.0f, 1.0f, (animation.time - 0.5f) / 0.5f);
            
            stateInfo.animationDelta = 1.0f;

            float const vmag = glm::length(physics.movementVector);

            if (stateChange &&
                vmag < 0.025) {
                physics.movementVector *= 0.1f;
            }
    
            if (vmag > 0.025f) {
                animation.clipA = clips.land;
                animation.clipB = clips.run;
                animation.blendFactor= math_util::lerp(0.0f, 1.0f, (animation.time / 0.2f));
            }

            if (animation.time >= 0.9f ||
                (animation.time >= 0.2f && vmag > 0.025f)) {
                stateInfo.state = CHARACTER_STATE_GROUNDED;
            } else if (stateInfo.groundedTimer >= 0.0f && input.jump) {
                stateInfo.state = CHARACTER_STATE_JUMPING;
            }
            break;
        }
        default:;
    }

    animation.blendFactor = glm::clamp(animation.blendFactor, 0.0f, 1.0f);

    stateInfo.previousState = state;
}

void CharacterSystem::updateCharacterInput(CharacterID characterID, float deltaTime) {
    auto const & input = m_characters.input[characterID];
    auto & transform = m_scene->getTransform(m_characters.entityIDs[characterID]);
    auto & physics = m_characters.physics[characterID];
    auto & stateInfo = m_characters.stateInfos[characterID];
    
    glm::vec3 targetMovement{0.0f};
    float influence;

    if (stateInfo.state == CHARACTER_STATE_JUMPING ||
        stateInfo.state == CHARACTER_STATE_FALLING) {
        // if character is airborne
        influence = 2.0f;
    } else if (stateInfo.state == CHARACTER_STATE_LANDING) {
        influence = 0.5f;
    } else {
        influence = 10.0f;
    }

    if (glm::length2(input.move) > 0.0f) {
        // if player performed any movement input
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
    
    physics.movementVector = glm::lerp(physics.movementVector, targetMovement, influence * deltaTime);

    physics.velocity.x = 2.0f * physics.movementVector.x;
    physics.velocity.z = 2.0f * physics.movementVector.z;
}
