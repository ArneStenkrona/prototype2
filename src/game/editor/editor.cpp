#include "editor.h"

Editor::Editor(Scene & scene, Input & input)
    : m_scene(scene),
      m_input(input),
      m_camera(input) {

}

void Editor::update(float deltaTime) {
    updateInput(deltaTime);
    render();
}

void Editor::updateInput(float deltaTime) {
    if (m_input.getKeyPress(INPUT_KEY::KEY_UP)) {
        printf("up\n");
        printf("%p\n", static_cast<void*>(&m_scene));
    }
    m_camera.update(deltaTime, true, true);

}

void Editor::render() {
    m_scene.renderScene(m_camera);
}
