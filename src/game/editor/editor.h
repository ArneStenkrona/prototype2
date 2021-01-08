#ifndef EDITOR_H
#define EDITOR_H

#include "src/game/scene/scene.h"

class Editor {
public:
    Editor(Scene & scene, Input & input);
    void update(float deltaTime);
private:
    void updateInput(float deltaTime);
    void render();

    EntityID m_selectedEntityID = -1;
    glm::vec3 m_selectedEntityPosition;
    glm::vec3 m_selectedMousePosition;    

    Scene & m_scene;
    Input & m_input;
    Camera m_camera;
};

#endif
