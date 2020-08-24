#include "player.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"

PlayerSystem::PlayerSystem(Input & input, Camera & camera, PhysicsSystem & physicsSystem) 
    : m_input(input), m_camera(camera), m_physicsSystem(physicsSystem) {}

void PlayerSystem::updatePlayer(Player & player, float deltaTime) {
    glm::vec3 moveInput = {0.0f, 0.0f, 0.0f};
    if (m_input.getKeyPress(INPUT_KEY::KEY_W)) {
        moveInput += glm::vec3{1.0f,0.0f,0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_S)) {
        moveInput -= glm::vec3{1.0f,0.0f,0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_A)) {
        moveInput -= glm::vec3{0.0f,0.0f,1.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_D)) {
        moveInput += glm::vec3{0.0f,0.0f,1.0f};    
    }

    player.jump = false;
    if (player.isGrounded && m_input.getKeyDown(INPUT_KEY::KEY_SPACE)) {
        player.jump = true;
    }

        // reference player fields with shorthands
    glm::vec3& gVel = player.gravityVelocity;
    bool& grounded = player.isGrounded;
    glm::vec3& groundNormal = player.groundNormal;
    bool& jump = player.jump;

    glm::vec3 targetVelocity{0.0f};
    
    glm::vec3 moveDir{0.0f};
    // if player performed any movement input
    if (glm::length2(moveInput) > 0.0f) {
        // compute look direction
        glm::vec3 cF = m_camera.getFront();
        glm::vec3 cR = m_camera.getRight();
        glm::vec3 lookDir = glm::normalize(moveInput.x * glm::vec3(cF.x, 0.0f, cF.z) + moveInput.z * glm::vec3(cR.x, 0.0f, cR.z));
        // rotate player model
        player.transform.rotation = 
                            math_util::safeQuatLookAt({0.0f,0.0f,0.0f}, lookDir, 
                                                      glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
        // compute movement direction
        glm::vec3 moveNormal = player.isGrounded ? player.groundNormal : glm::vec3{0.0f, 1.0f, 0.0f};

        moveDir = glm::normalize(glm::cross(moveNormal, 
                                 glm::cross(lookDir, moveNormal)));

        if (m_input.getKeyPress(INPUT_KEY::KEY_LEFT_SHIFT)) {
            targetVelocity = 0.2f * moveDir;
        } else {
            targetVelocity = 0.1f * moveDir;
        }
    }

    // add friction
    float gnDotUp = glm::dot(groundNormal, glm::vec3{0.0f,1.0f,0.0f});
    if (grounded && gnDotUp < 0.72f) {
        float slideFactor = 5.0f;
        gVel += slideFactor * (1.0f - gnDotUp) * deltaTime * glm::normalize(glm::cross(groundNormal, 
                                                glm::cross(glm::vec3{0.0f,-1.0f,0.0f}, groundNormal)));
    } else if (!grounded) {
        float gAcc = m_physicsSystem.getGravity() * deltaTime;
        gVel += glm::vec3{0.0f, -1.0f, 0.0f} * gAcc;
    } else {
        gVel -= player.groundNormal * deltaTime;
    }

    // jump
    if (jump) {
        gVel += 0.5f * glm::vec3{0.0f, 1.0f, 0.0f};
    }

    player.velocity = glm::lerp(player.velocity, targetVelocity, 10.0f * deltaTime);
    // TODO: sync walk and run animation
    float vmag = glm::length(player.velocity);
    if (vmag > 0.1f) {
        player.animationA = player.walkAnimationIndex;
        player.animationB = player.runAnimationIndex;
        player.animationBlendFactor = (vmag - 0.1f) / 0.1f;
    } else {
        player.animationA = player.idleAnimationIndex;
        player.animationB = player.walkAnimationIndex;
        player.animationBlendFactor = vmag / 0.1f;
    }
};
