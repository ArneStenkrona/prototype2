#ifndef PRT_CHARACTER_H
#define PRT_CHARACTER_H

#include "src/game/system/physics/collider_tag.h"
#include "src/game/system/animation/animation_system.h"

#include "src/game/component/component.h"

#include <glm/glm.hpp>

class Scene;

enum CharacterType : int {
    CHARACTER_TYPE_NONE,
    CHARACTER_TYPE_PLAYER,
    CHARACTER_TYPE_NPC
};

struct CharacterPhysics {
    glm::vec3   forward = {1.0f, 0.0f, 0.0f};
    glm::vec3   velocity = {0.0f, 0.0f, 0.0f};
    float       mass = 1.0f;
    glm::vec3   movementVector = {0.0f, 0.0f, 0.0f};
    glm::vec3   groundNormal;
    ColliderTag colliderTag;
    bool        isGrounded = false;
};

struct CharacterInput {
    glm::vec2 move = {0.0f, 0.0f};
    bool      run = false;
    bool      jump = false;
    bool      holdjump = false;
    bool      attack = false;
};

enum CharacterState {
    CHARACTER_STATE_IDLE,
    CHARACTER_STATE_WALKING,
    CHARACTER_STATE_RUNNING,
    CHARACTER_STATE_JUMPING,
    CHARACTER_STATE_FALLING,
    CHARACTER_STATE_LANDING,
    CHARACTER_STATE_LANDING_MILDLY,
    CHARACTER_STATE_ROLLING,
    CHARACTER_STATE_SLASH1,
    CHARACTER_STATE_SLASH2,
    CHARACTER_STATE_MIDAIR_SLASH1,
    CHARACTER_STATE_MIDAIR_SLASH2,
    TOTAL_NUM_CHARACTER_STATE,
};

struct CharacterStateAttributeInfo {
    char      animationName[64] = {0};
    float     animationSpeed = 1.0f;
    bool      resetAnimationTime = true;
    bool      loopAnimation = true;
    bool      canTurn = true;
    float     movementSpeed = 1.0f;
    glm::vec3 impulse{0.0f};
};

struct Equipment {
    bool equipped = true;
    int      boneIndex;
    EntityID entity;
    Transform offset;
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
    float          const & getMovementSpeed() const { return m_movementSpeed; }
    bool           const &  getCanTurn() const { return m_canTurn; }
    bool           const &  getHasJumped() const { return m_hasJumped; }
    bool           const &  getStateChange() const { return m_stateChange; }
private:
    CharacterState m_state = CHARACTER_STATE_IDLE;
    CharacterState m_previousState = CHARACTER_STATE_IDLE;
    float          m_groundedTimer = 0.0f;
    float          m_stateTimer = -1.0f;
    float          m_transitionTimer = 0.0f;
    float          m_transitionLength = 0.0f;
    float          m_animationSpeed = 0.0f;
    float          m_movementSpeed = 1.0f;
    bool           m_canTurn = true;
    bool           m_hasJumped;
    bool           m_stateChange = true;

    void update(float deltaTime,
                AnimationComponent & animation,
                CharacterPhysics & physics);

    static CharacterStateAttributeInfo getStateAttributeInfo(CharacterState state, CharacterPhysics const & physics);

    friend class CharacterAttributeInfo;
};

class CharacterAttributeInfo {
public:
    CharacterType           type = CHARACTER_TYPE_NONE;
    CharacterStateInfo      stateInfo;
    prt::vector<Equipment>  equipment;

    void updateState(float deltaTime, 
                     AnimationComponent & animation,
                     CharacterPhysics & physics);
    void updateEquipment(EntityID entityID, Scene & scene, AnimationSystem const & animationSystem);
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
