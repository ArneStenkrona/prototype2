#ifndef EDITOR_H
#define EDITOR_H

#include "editor_gui.h"
#include "src/game/scene/scene.h"

class Editor {
public:
    Editor(Scene & scene, Input & input, int width, int height);
    void update(float deltaTime, int width, int height);
private:
    void updateInput(float deltaTime);
    void render();

    EntityID m_selectedEntityID = -1;
    glm::vec3 m_selectedEntityPosition;
    glm::vec3 m_selectedMousePosition;    

    Scene & m_scene;
    Input & m_input;
    Camera m_camera;

    EditorGui m_gui;
};

#endif
