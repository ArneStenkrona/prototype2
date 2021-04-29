#ifndef PRT_CHARACTER_H
#define PRT_CHARACTER_H

#include "src/game/system/physics/collider_tag.h"
#include "src/game/system/animation/animation_system.h"

#include <glm/glm.hpp>

enum CharacterType : int {
    CHARACTER_TYPE_NONE,
    CHARACTER_TYPE_PLAYER,
    CHARACTER_TYPE_NPC
};

struct CharacterPhysics {
    glm::vec3   velocity = {0.0f, 0.0f, 0.0f};
    float       mass = 1.0f;
    glm::vec3   movementVector = {0.0f, 0.0f, 0.0f};
    glm::vec3   groundNormal;
    ColliderTag colliderTag;
    bool        isGrounded = false;
    bool        isJumping = false;
    bool        isGliding = false;
    // float       airTime = 0.0f;
};

struct CharacterInput {
    glm::vec2 move = {0.0f, 0.0f};
    bool      run = false;
    bool      jump = false;
    bool      holdjump = false;
};

struct CharacterAnimationClips {
    int idle;
    int walk;
    int run;
    int jump;
    int fall;
    int glide;
    int land;
};

enum CharacterState {
    CHARACTER_STATE_IDLE,
    CHARACTER_STATE_WALKING,
    CHARACTER_STATE_RUNNING,
    CHARACTER_STATE_JUMPING,
    CHARACTER_STATE_FALLING,
    CHARACTER_STATE_LANDING,
    TOTAL_NUM_CHARACTER_STATE
};

struct CharacterStateAttributeInfo {
    uint32_t animationClip = 0;
    float    animationSpeed = 1.0f;
    bool     resetAnimationTime = true;
    bool     loopAnimation = true;
    float    inputInfluence = 1.0f;
};

class CharacterAttributeInfo;

class CharacterStateInfo {
public:
    void transitionState(CharacterState newState, float transitionTime);

    void resetStateChange() { m_stateChange = false; }

    CharacterState const & getState() const { return m_state; };
    CharacterState const & getPreviousState() const { return m_previousState; }
    float          const & getGroundedTimer() const { return m_groundedTimer; }
    float          const & getStateTimer() const { return m_stateTimer; }
    float          const & getTransitionTimer() const { return m_transitionTimer; }
    float          const & getTransitionLength() const { return m_transitionLength; }
    float          const & getAnimationSpeed() const { return m_animationSpeed; }
    bool           const &  getHasJumped() const { return m_hasJumped; }
    bool           const &  getStateChange() const { return m_stateChange; }
private:
    CharacterState m_state;
    CharacterState m_previousState;
    float          m_groundedTimer = 0.0f;
    float          m_stateTimer = -1.0f;
    float          m_transitionTimer = 0.0f;
    float          m_transitionLength = 0.0f;
    float          m_animationSpeed = 0.0f;
    bool           m_hasJumped;
    bool           m_stateChange = false;

    void update(float deltaTime, 
                BlendedAnimation & animation,
                CharacterPhysics & physics,
                CharacterAnimationClips const & clips);

    static CharacterStateAttributeInfo getStateAttributeInfo(CharacterState state,
                                                             CharacterAnimationClips const & clips);

    friend class CharacterAttributeInfo;
};

class CharacterAttributeInfo {
public:
    CharacterType type = CHARACTER_TYPE_NONE;
    CharacterStateInfo stateInfo;
    CharacterAnimationClips clips;
    void update(float deltaTime, 
                BlendedAnimation & animation,
                CharacterPhysics & physics) {
        stateInfo.update(deltaTime, animation, physics, clips);
    }
private:
};

template<size_t N>
struct Characters {
    enum { maxSize = N };
    CharacterID size = 0;

    EntityID                entityIDs[N];
    CharacterAttributeInfo  attributeInfos[N];
    CharacterPhysics        physics[N];
    CharacterInput          input[N];
};

#endif
