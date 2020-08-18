#include "player.h"

#include "src/util/math_util.h"

PlayerSystem::PlayerSystem(Input & input, Camera & camera, PhysicsSystem & physicsSystem) 
    : m_input(input), m_camera(camera), m_physicsSystem(physicsSystem) {}

void PlayerSystem::updatePlayer(Player & player, float deltaTime) {
    player.state = Player::State::IDLE;
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

    if (glm::length2(moveInput) > 0.0f) {
        player.state = Player::State::WALKING;
    }

    if (player.state == Player::State::WALKING &&
        m_input.getKeyPress(INPUT_KEY::KEY_LEFT_SHIFT)) {
        player.state = Player::State::RUNNING;
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
    
    glm::vec3 moveDir;
    // if player performed any movement input
    if (player.state == Player::State::WALKING || 
        player.state == Player::State::RUNNING) {
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

    // handle states
    switch (player.state) {
        case Player::State::IDLE:
            player.velocity = 0.0f * moveDir;
            player.currentAnimation = player.animationIdleIndex;
            break;
        case Player::State::WALKING:
            player.velocity = 0.1f * moveDir;
            player.currentAnimation = player.animationWalkIndex;
            break;
        case Player::State::RUNNING:
            player.velocity = 0.2f * moveDir;
            player.currentAnimation = player.animationRunIndex;
            break;
        default:
        break;
    }
};
