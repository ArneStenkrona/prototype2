#include "editor_gui.h"

#include "imgui/imgui.h"

#include <cstdio>

void EditorGui::update(Input & input, int w, int h, float deltaTime) {
    updateInput(input, w, h, deltaTime);
    newFrame();
}

void EditorGui::updateInput(Input & input, int w, int h, float deltaTime) {
    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DeltaTime = deltaTime;

    float dpiScaleFactor = 2.0f;

    double mouseX, mouseY;
    input.getCursorPos(mouseX, mouseY);
    io.MousePos = ImVec2(mouseX / dpiScaleFactor, mouseY / dpiScaleFactor);
    io.MouseDown[0] = input.getKeyPress(INPUT_KEY::KEY_MOUSE_LEFT) == GLFW_PRESS;
    io.MouseDown[1] = input.getKeyPress(INPUT_KEY::KEY_MOUSE_RIGHT) == GLFW_PRESS;
}

void EditorGui::newFrame() {
    // begin
    ImGui::NewFrame();

    ImGui::Begin("Scene");
    ImGui::Text("Hello");
    ImGui::End();
    
    ImGui::Begin("Scene2");
    ImGui::Text("Hello");
    ImGui::End();    
    // end
    ImGui::Render();
}
