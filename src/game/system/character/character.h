#ifndef PRT_CHARACTER_H
#define PRT_CHARACTER_H

#include "src/game/system/physics/collider_tag.h"

#include <glm/glm.hpp>

enum CharacterType : uint16_t {
    CHARACTER_TYPE_NONE,
    CHARACTER_TYPE_PLAYER,
    CHARACTER_TYPE_NPC
};

// typedef uint16_t CharacterIndex;
// struct CharacterTag {
//     CharacterIndex index;
//     CharacterType type = CharacterType::CHARACTER_TYPE_NONE;
//     friend bool operator== (CharacterTag const & c1, CharacterTag const & c2) {
//         return (c1.index == c2.index &&
//                 c1.type == c2.type);
//     }
//     friend bool operator!= (CharacterTag const & c1, CharacterTag const & c2)  {
//         return !(c1 == c2);
//     }
// };

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
    uint32_t animationClip;
    float animationDelta;
    bool resetAnimationTime;
    bool loopAnimation;
};

struct CharacterStateInfo {
    CharacterState state;
    CharacterState previousState;
    float          groundedTimer = 0.0f;
    float          stateTimer = -1.0f;
    float          transitionTimer = 0.0f;
    float          transitionLength = 0.0f;
    float          animationDelta = 0.0f;
    bool           hasJumped;
    bool           stateChange = false;

    void transitionState(CharacterState newState, float transitionTime) {
        stateChange = newState != state;

        previousState = state;
        state = newState;

        transitionTimer = 0.0f;
        transitionLength = transitionTime;
    }
};

struct CharacterAttributeInfo {
    CharacterType type = CHARACTER_TYPE_NONE;
    CharacterStateInfo stateInfo;
};

template<size_t N>
struct Characters {
    enum { maxSize = N };
    CharacterID size = 0;

    EntityID                entityIDs[N];
    CharacterAttributeInfo  attributeInfos[N];
    CharacterPhysics        physics[N];
    CharacterInput          input[N];
    CharacterAnimationClips animationClips[N];
    float                   animationSpeeds[N];
};

#endif
