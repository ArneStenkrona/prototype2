#ifndef PRT_COMPONENT_H
#define PRT_COMPONENT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "src/game/system/physics/collider_tag.h"

/* Transform */
struct Transform {
    glm::vec3 position = {0.0f,0.0f,0.0f};
    glm::quat rotation = {1.0f,0.0f,0.0f,0.0f};
    glm::vec3 scale = {1.0f,1.0f,1.0f};

    glm::mat4 transformMatrix() const {
        glm::mat4 scaleM = glm::scale(scale);
        glm::mat4 rotateM = glm::toMat4(glm::normalize(rotation));
        glm::mat4 translateM = glm::translate(glm::mat4(1.0f), position);
        return translateM * rotateM * scaleM;
    }
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
    CHARACTER_STATE_GROUNDED,
    CHARACTER_STATE_JUMPING,
    CHARACTER_STATE_FALLING,
    CHARACTER_STATE_GLIDING,
    CHARACTER_STATE_LANDING,
    TOTAL_NUM_CHARACTER_STATE
};

struct CharacterStateInfo {
    CharacterState state;
    CharacterState previousState;
    float          groundedTimer = 0.0f;
    float          stateTimer = -1.0f;
    float          animationDelta = 0.0f;
    bool           hasJumped;
};

#endif
