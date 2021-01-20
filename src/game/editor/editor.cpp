#include "editor.h"

#include "glm/gtx/string_cast.hpp"

Editor::Editor(Scene & scene, Input & input, int width, int height)
    : m_scene(scene),
      m_input(input),
      m_camera(input),
      m_gui(width, height) {
}

void Editor::update(float deltaTime, int width, int height) {
    updateInput(deltaTime);
    m_gui.update(m_input, width, height, deltaTime, m_scene);
    render();
}

void Editor::updateInput(float deltaTime) {
    if (m_input.getKeyDown(INPUT_KEY::KEY_MOUSE_RIGHT)) {
        m_selectedEntityID = m_scene.m_renderResult.entityID;

        double x, y;
        m_input.getCursorPos(x, y);

        int w, h;
        m_scene.m_gameRenderer.getWindowSize(w, h);

        m_selectedMousePosition.x = 2.0 * (x / w) - 1.0; 
        m_selectedMousePosition.y = 1.0 - 2.0 * (y / h); 

        m_selectedMousePosition.z = m_scene.m_renderResult.mouseDepth;

        m_selectedEntityPosition = m_scene.m_entities.transforms[m_selectedEntityID].position;

        m_scene.m_colliderUpdateSet.insert(m_selectedEntityID);
    }

    if (m_selectedEntityID != -1 && m_input.getKeyPress(INPUT_KEY::KEY_MOUSE_RIGHT)) {
        double x, y;

        m_input.getCursorPos(x, y);

        int w, h;
        m_scene.m_gameRenderer.getWindowSize(w, h);

        x = 2.0 * (x / w) - 1.0; 
        y = 1.0 - 2.0 * (y / h); 

        double dx = x - m_selectedMousePosition.x;
        double dy = y - m_selectedMousePosition.y;

        glm::mat4 invProj = glm::inverse(m_camera.getProjectionMatrix() * m_camera.getViewMatrix());

        glm::vec4 selectedWorldPos = invProj * glm::vec4(m_selectedMousePosition, 1.0f);
        selectedWorldPos = selectedWorldPos /selectedWorldPos.w;

        glm::vec4 mouseScreenPos = glm::vec4(m_selectedMousePosition, 1.0f);

        glm::vec4 newWorldPos = invProj * (mouseScreenPos + glm::vec4(dx, dy, 0.0f, 0.0f));
        newWorldPos = newWorldPos / newWorldPos.w;

        glm::vec3 dPos = glm::vec3(newWorldPos - selectedWorldPos);

        m_scene.m_entities.transforms[m_selectedEntityID].position = m_selectedEntityPosition + dPos;
    } else {
        m_selectedEntityID = -1;
    }
    
    m_camera.update(deltaTime, true, true);

}

void Editor::render() {
    m_scene.renderScene(m_camera);
}
