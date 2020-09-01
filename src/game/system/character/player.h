#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>

#include "src/game/component/component.h"
#include "src/game/system/physics/physics_system.h"
#include "src/graphics/camera/camera.h"
#include "src/system/input/input.h"

class PlayerController {
public:
    PlayerController(Input & input, Camera & camera);

    void updateInput(CharacterInput & input);
private:
    Input & m_input;
    Camera & m_camera;
};

#endif