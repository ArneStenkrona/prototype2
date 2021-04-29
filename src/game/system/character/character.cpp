#include "character.h"

#include <glm/glm.hpp>

void CharacterStateInfo::update(float deltaTime, 
                                BlendedAnimation & animation,
                                CharacterPhysics & physics,
                                CharacterAnimationClips const & clips) {
    // behaviour
    if (physics.isGrounded) {
        m_groundedTimer = 0.0f;
    } else {
        m_groundedTimer += deltaTime;
    }
                                    
    CharacterStateAttributeInfo attributeInfo = getStateAttributeInfo(m_state, clips);
    if (m_stateChange) {
        m_stateTimer = 0.0f;
        if (attributeInfo.resetAnimationTime) animation.time = 0.0f;
    } else {
        m_stateTimer += deltaTime;
        animation.time += m_animationSpeed * deltaTime;
        if (!attributeInfo.loopAnimation) animation.time = glm::clamp(animation.time, 0.0f, 1.0f);
    }

    m_transitionTimer += deltaTime;
    
    animation.clipA = attributeInfo.animationClip;
    animation.blendFactor = 0.0f;
    m_animationSpeed = attributeInfo.animationSpeed;

    if (m_transitionTimer < m_transitionLength) {
        CharacterStateAttributeInfo prevAttributeInfo = getStateAttributeInfo(m_previousState, clips);
        animation.clipB = prevAttributeInfo.animationClip;
        float normTime = m_transitionTimer / m_transitionLength;
        animation.blendFactor = glm::clamp(1.0f - normTime, 0.0f, 1.0f);
        m_animationSpeed = glm::mix(prevAttributeInfo.animationSpeed, 
                                            attributeInfo.animationSpeed,
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

CharacterStateAttributeInfo CharacterStateInfo::getStateAttributeInfo(CharacterState state,
                                                                      CharacterAnimationClips const & clips) {
    CharacterStateAttributeInfo attributeInfo;
    switch (state) {
        case CHARACTER_STATE_IDLE: {
            attributeInfo.animationClip = clips.idle;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            break;
        }
        case CHARACTER_STATE_WALKING: {
            attributeInfo.animationClip = clips.walk;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            break;
        }
        case CHARACTER_STATE_RUNNING: {
            attributeInfo.animationClip = clips.run;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = false;
            attributeInfo.loopAnimation = true;
            break;
        }
        case CHARACTER_STATE_JUMPING: {
            attributeInfo.animationClip = clips.jump;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            break;
        }
        case CHARACTER_STATE_FALLING: {
            attributeInfo.animationClip = clips.fall;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            break;
        }
        case CHARACTER_STATE_LANDING: {
            attributeInfo.animationClip = clips.land;
            attributeInfo.animationSpeed = 1.0f;
            attributeInfo.resetAnimationTime = true;
            attributeInfo.loopAnimation = false;
            break;
        }
        default: {}
    }

    return attributeInfo;
}
