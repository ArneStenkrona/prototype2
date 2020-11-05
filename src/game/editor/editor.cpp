#include "editor.h"

Editor::Editor(Scene & scene, Input & input)
    : m_scene(scene),
      m_input(input),
      m_camera(input) {

}

void Editor::update() {
    updateInput();
}

void Editor::updateInput() {
    
}
