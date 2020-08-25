#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>

#include "src/game/component/component.h"
#include "src/game/system/physics/physics_system.h"
#include "src/graphics/camera/camera.h"
#include "src/system/input/input.h"

struct Player {
    uint32_t modelID;
    uint32_t animationA;
    uint32_t animationB;
    Transform transform;
    glm::vec3 velocity;
    glm::vec3 gravityVelocity;
    uint32_t ellipsoidColliderID;
    bool isGrounded;
    glm::vec3 groundNormal;
    bool jump;

    float animationBlendFactor = 0.0f;
    float animationTimer = 0.0f;

    uint32_t idleAnimationIndex;
    uint32_t walkAnimationIndex;
    uint32_t runAnimationIndex;
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