#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>

#include "src/game/component/component.h"
#include "src/game/system/physics/physics_system.h"
#include "src/graphics/camera/camera.h"
#include "src/system/input/input.h"

struct Player {
    uint32_t modelID;
    uint32_t currentAnimation;
    Transform transform;
    glm::vec3 velocity;
    glm::vec3 direction;
    glm::vec3 gravityVelocity;
    uint32_t ellipsoidColliderID;
    bool isGrounded;
    glm::vec3 groundNormal;
    bool jump;

    enum class State {
        IDLE,
        WALKING,
        RUNNING
    };
    State state = State::IDLE;

    uint32_t animationIdleIndex = 0;
    uint32_t animationWalkIndex = 0;
    uint32_t animationRunIndex = 0;
};

class PlayerSystem {
public:
    PlayerSystem(Input & input, Camera & camera, PhysicsSystem & physicsSystem);
    void updatePlayer(Player & player, float deltaTime);
private:
    Input & m_input;
    Camera & m_camera;
    PhysicsSystem & m_physicsSystem;
};

#endif