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

CharacterID CharacterSystem::addCharacter(EntityID entityID, ColliderTag tag) { 
    assert(m_characters.size < m_characters.maxSize && "Character amount exceeded!");

    m_characters.entityIDs[m_characters.size] = entityID;
    m_characters.physics[m_characters.size].colliderTag = tag;

    ++m_characters.size; 
    return m_characters.size - 1; 
}

void CharacterSystem::addEquipment(CharacterID characterID, int boneIndex, EntityID equipment, Transform offset) {
    m_characters.attributeInfos[characterID].equipment.push_back({});
    m_characters.attributeInfos[characterID].equipment.back().entity = equipment;
    m_characters.attributeInfos[characterID].equipment.back().offset = offset;
    m_characters.attributeInfos[characterID].equipment.back().boneIndex = boneIndex;
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
        m_characters.attributeInfos[i].updateEquipment(m_characters.entityIDs[i], *m_scene, m_animationSystem);
    }
}

void CharacterSystem::updateCharacter(CharacterID characterID, float deltaTime) {
    auto & attributeInfo = m_characters.attributeInfos[characterID];
    auto & physics = m_characters.physics[characterID];
    auto & animation = m_animationSystem.getAnimationComponent(m_characters.entityIDs[characterID]);

    updateCharacterInput(characterID, deltaTime);

    attributeInfo.updateState(deltaTime, animation, physics);

    switch (attributeInfo.stateInfo.getState()) {
        case CHARACTER_STATE_JUMPING: {
            if (attributeInfo.stateInfo.getStateChange()) {
                physics.velocity.y = 0.35f;
            }
            
            break;
        }
        default: {}
    }
    
    setStateTransitions(characterID);
}

void CharacterSystem::updateCharacterInput(CharacterID characterID, float /*deltaTime*/) {
    auto const & input = m_characters.input[characterID];
    auto & transform = m_scene->getTransform(m_characters.entityIDs[characterID]);
    auto & physics = m_characters.physics[characterID];
    auto & stateInfo = m_characters.attributeInfos[characterID].stateInfo;
    
    glm::vec3 targetMovement{0.0f};

    // check if character performed any movement input
    if (glm::length2(input.move) > 0.0f) {
        glm::vec3 const origin{0.0f, 0.0f, 0.0f}; 
        glm::vec3 const up{0.0f, 1.0f, 0.0f}; 
        glm::vec3 const altUp{0.0f, 0.0f, 1.0f}; 
        // compute movement plane
        glm::vec3 const moveNormal = physics.isGrounded ? physics.groundNormal : up;

        glm::vec3 const newDir = glm::normalize(glm::vec3{input.move.x,  0.0f, input.move.y});

        glm::vec3 const prevDir = glm::length2(physics.movementVector) > 0.0f ?
                                     glm::normalize(glm::vec3{physics.movementVector.x, 0.0f, physics.movementVector.z}) :
                                     newDir;


        glm::vec3 const lookDir = glm::dot(prevDir, newDir) > -0.9f ?
                                         glm::mix(prevDir, newDir, 0.5f) :
                                         newDir;

        // rotate character
        transform.rotation = math_util::safeQuatLookAt(origin, lookDir, up, altUp);

        glm::vec3 const moveDir = glm::normalize(glm::cross(moveNormal, glm::cross(lookDir, moveNormal)));

        targetMovement = moveDir * stateInfo.getMovementSpeed();
    }

    physics.movementVector = targetMovement;

    physics.velocity.x = physics.movementVector.x;
    physics.velocity.z = physics.movementVector.z;
}

void CharacterSystem::setStateTransitions(CharacterID characterID) {
    auto const & input = m_characters.input[characterID];
    auto & stateInfo = m_characters.attributeInfos[characterID].stateInfo;
    auto & physics = m_characters.physics[characterID];
    auto & animation = m_animationSystem.getAnimationComponent(m_characters.entityIDs[characterID]);

    stateInfo.resetStateChange();

    switch (stateInfo.getState()) {
        case CHARACTER_STATE_IDLE: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            }
            if (glm::length2(input.move) > 0.0f) {
                stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
            }
            break;
        }
        case CHARACTER_STATE_WALKING: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
               stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            } else if (glm::length2(input.move) > 0.0f) {
                if (input.run) stateInfo.transitionState(CHARACTER_STATE_RUNNING, 0.1f);
            } else {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
            }
            break;
        }
        case CHARACTER_STATE_RUNNING: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            } else if (glm::length2(input.move) > 0.0f) {
                if (!input.run) stateInfo.transitionState(CHARACTER_STATE_WALKING, 0.1f);
            } else {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
            }
            break;
        }
        case CHARACTER_STATE_JUMPING: {
            if (physics.isGrounded && stateInfo.getStateTimer() > 0.1f) {
                stateInfo.transitionState(CHARACTER_STATE_LANDING, 0.0f);
            } else if (animation.clipA.isCompleted()) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.3f);
            }
            break;
        }
        case CHARACTER_STATE_FALLING: {
            if (physics.isGrounded) {
                stateInfo.transitionState(CHARACTER_STATE_LANDING, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_LANDING: {
            if (glm::length2(input.move) > 0.0f) {
                stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
            } else if (animation.clipA.isCompleted()) {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.3f);
            } else if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            }
            break;
        }

        default: {}
    }
}
