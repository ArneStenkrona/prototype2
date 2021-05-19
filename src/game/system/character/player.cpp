#include "player.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"

PlayerController::PlayerController(Input & input, Camera & camera) 
    : m_input(input), m_camera(camera) {}

void PlayerController::updateInput(CharacterInput & input) {
    glm::vec2 rawInput{0.0f, 0.0f};
    if (m_input.getKeyPress(INPUT_KEY::KEY_W)) {
        rawInput+= glm::vec2{1.0f, 0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_S)) {
        rawInput -= glm::vec2{1.0f, 0.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_A)) {
        rawInput -= glm::vec2{0.0f, 1.0f};
    }
    if (m_input.getKeyPress(INPUT_KEY::KEY_D)) {
        rawInput += glm::vec2{0.0f, 1.0f};
    }
    // project input according to camera
    if (glm::length2(rawInput) > 0.0f) {
        // compute look direction
        glm::vec3 cF = m_camera.getFront();
        glm::vec3 cR = m_camera.getRight();
        input.move = glm::normalize(rawInput.x * glm::vec2{cF.x, cF.z} + rawInput.y * glm::vec2{cR.x, cR.z});

    } else {
        input.move = {0.0f, 0.0f};
    }

    input.jump = m_input.getKeyDown(INPUT_KEY::KEY_SPACE);
    input.holdjump = m_input.getKeyPress(INPUT_KEY::KEY_SPACE);

    input.run = !m_input.getKeyPress(INPUT_KEY::KEY_LEFT_SHIFT);

    input.attack = m_input.getKeyDown(INPUT_KEY::KEY_ENTER);
}
