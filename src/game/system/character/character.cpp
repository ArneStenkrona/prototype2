#include "character.h"

#include "src/game/scene/scene.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <string.h>

void CharacterStateInfo::update(float deltaTime, 
                                AnimationComponent & animation,
                                CharacterPhysics & physics) {
    if (physics.isGrounded) {
        m_groundedTimer = 0.0f;
    } else {
        m_groundedTimer += deltaTime;
    }

    m_stateTimer += deltaTime;
                                    
    CharacterStateAttributeInfo attributeInfo = getStateAttributeInfo(m_state);
    CharacterStateAttributeInfo prevAttributeInfo = getStateAttributeInfo(m_previousState);

    if (m_stateChange) {
        animation.clipB.setClip(prevAttributeInfo.animationName);
        animation.clipB.m_loop = prevAttributeInfo.loopAnimation;
        animation.clipB.m_playBackSpeed = prevAttributeInfo.animationSpeed;
        animation.clipB.m_time = animation.clipA.m_time;

        m_stateTimer = 0.0f;
        if (attributeInfo.resetAnimationTime) {
            animation.clipA.resetClip();
        }

        animation.clipA.setClip(attributeInfo.animationName);
        animation.clipA.m_loop = attributeInfo.loopAnimation;
        animation.clipA.m_playBackSpeed = attributeInfo.animationSpeed;
    }


    animation.blendFactor = 0.0f;

    m_transitionTimer += deltaTime;

    m_canTurn = attributeInfo.canTurn;
    m_movementSpeed = attributeInfo.movementSpeed;

    if (m_transitionTimer < m_transitionLength) {
        float normTime = m_transitionTimer / m_transitionLength;
        animation.blendFactor = glm::clamp(1.0f - normTime, 0.0f, 1.0f);
        m_animationSpeed = glm::mix(prevAttributeInfo.animationSpeed, 
                                    attributeInfo.animationSpeed,
                                    normTime);

        m_movementSpeed = glm::mix(prevAttributeInfo.movementSpeed, 
                                   attributeInfo.movementSpeed,
                                   normTime);
            
    }
}

void CharacterStateInfo::transitionState(CharacterState newState, float transitionTime) {
    m_stateChange = newState != m_state;

    m_previousState = m_state;
    m_state = newState;

    m_transitionTimer = 0.0f;
    m_transitionLength = transitionTime;
}

CharacterStateAttributeInfo CharacterStateInfo::getStateAttributeInfo(CharacterState state) {
    CharacterStateAttributeInfo attributeInfo;
    switch (state) {
        case CHARACTER_STATE_IDLE: {
            strcpy(attributeInfo.animationName, "idle");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            attributeInfo.movementSpeed = 0.0f;
            attributeInfo.canTurn = true;
            break;
        }
        case CHARACTER_STATE_WALKING: {
            strcpy(attributeInfo.animationName, "walk");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            attributeInfo.movementSpeed = 0.1f;
            attributeInfo.canTurn = true;
            break;
        }
        case CHARACTER_STATE_RUNNING: {
            strcpy(attributeInfo.animationName, "run");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            attributeInfo.movementSpeed = 0.2f;
            attributeInfo.canTurn = true;
            break;
        }
        case CHARACTER_STATE_JUMPING: {
            strcpy(attributeInfo.animationName, "jump");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            attributeInfo.movementSpeed = 0.2f;
            attributeInfo.canTurn = true;
            break;
        }
        case CHARACTER_STATE_FALLING: {
            strcpy(attributeInfo.animationName, "fall");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            attributeInfo.movementSpeed = 0.2f;
            attributeInfo.canTurn = true;
            break;
        }
        case CHARACTER_STATE_LANDING: {
            strcpy(attributeInfo.animationName, "land");
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            attributeInfo.movementSpeed = 0.1f;
            attributeInfo.canTurn = true;
            break;
        }
        default: {}
    }

    return attributeInfo;
}

void CharacterAttributeInfo::updateState(float deltaTime, 
                                         AnimationComponent & animation,
                                         CharacterPhysics & physics) {
            stateInfo.update(deltaTime, animation, physics);
}

void CharacterAttributeInfo::updateEquipment(EntityID entityID, Scene & scene, AnimationSystem const & animationSystem) {
    Transform const & tform = scene.getTransform(entityID);

    for (Equipment const & equip : equipment) {
        if (equip.entity == EntityID(-1)) {
            continue;
        }
        glm::mat4 const & boneTform = animationSystem.getCachedTransformation(entityID, equip.boneIndex);
        glm::mat4 nodeTform = scene.getModel(entityID).getBoneTransform(equip.boneIndex);
        glm::mat4 finalTform = tform.transformMatrix() * boneTform * nodeTform * equip.offset.transformMatrix();
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(finalTform, scale, rotation, translation, skew, perspective);

        Transform & rightTform = scene.getTransform(equip.entity);
        rightTform.position = translation;
        rightTform.rotation = rotation;
        rightTform.scale = equip.offset.scale;
    }
}
