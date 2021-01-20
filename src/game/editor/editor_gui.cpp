#include "editor_gui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_glfw.h"

#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/string_cast.hpp"

#include <cstdio>

EditorGui::EditorGui(GLFWwindow * window, float width, float height) {
    init(width, height);
    ImGui_ImplGlfw_InitForVulkan(window, true);
}

void EditorGui::update(Input & input, int width, int height, float deltaTime, Scene & scene, EditorUpdate const & updatePackage) {
    updateInput(input, width, height, deltaTime);

    if (updatePackage.selectedEntity != -1) {
        selectedEntity = updatePackage.selectedEntity;
    }

    newFrame(scene);
}

void EditorGui::init(float width, float height) {
    // Color scheme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    style.ScaleAllSizes(dpiScaleFactor);
    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DisplaySize = ImVec2{width, height};

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void EditorGui::updateInput(Input & input, int width, int height, float deltaTime) {
    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = deltaTime;

    double mouseX, mouseY;
    input.getCursorPos(mouseX, mouseY);
    io.MousePos = ImVec2(mouseX, mouseY);
    io.MouseDown[0] = input.getKeyPress(INPUT_KEY::KEY_MOUSE_LEFT);
    io.MouseDown[1] = input.getKeyPress(INPUT_KEY::KEY_MOUSE_RIGHT);

}

void EditorGui::newFrame(Scene & scene) {
    ImGui::NewFrame();

    buildEditor(scene);

    ImGui::Render();
}

void EditorGui::buildEditor(Scene & scene) {
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static auto first_time = true;
        if (first_time)
        {
            first_time = false;

            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
            //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
            //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
            auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
            auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

            // we now dock our windows into the docking node we made above
            ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
            ImGui::DockBuilderDockWindow("Scene", dock_id_left);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    ImGui::End();

    Entities & entities = scene.getEntities();

    ImGui::Begin("Scene");
    entityList(scene, entities);
    ImGui::End();

    ImGui::Begin("Inspector");
    entityInfo(scene, entities);
    ImGui::End();
}

// TODO: cache this operation
void EditorGui::entityList(Scene & /*scene*/, Entities & entities) {
    int nEntities = entities.size();
    prt::vector<char*> names;
    names.resize(entities.size());

    for (EntityID i = 0; i < entities.size(); ++i) {
        names[i] = entities.names[i];
    }

    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 20);
    ImGui::ListBox("", &selectedEntity, names.data(), nEntities);
}

void EditorGui::entityInfo(Scene & scene, Entities & entities) {
    if (selectedEntity == -1) return;

    char * name = entities.names[selectedEntity];
    
    ImGui::PushID(selectedEntity);
    ImGui::PushItemWidth(200);

    ImGui::InputText("Name", name, Entities::SIZE_STR);

    ImGui::PopID();
    ImGui::PopItemWidth();
    
    showTransform(scene, entities.transforms[selectedEntity]);

    if (scene.hasModel(selectedEntity)) {
        showModel(scene, scene.getModel(selectedEntity));
    }
}

void EditorGui::showTransform(Scene & scene, Transform & transform) {
    beginGroupPanel("Transform");
    
    glm::vec3 & position = transform.position;
    glm::quat & rotation = transform.rotation;
    glm::vec3 & scale = transform.scale;

    ImGui::PushID(selectedEntity);
    ImGui::PushItemWidth(400);

    float* vecp = reinterpret_cast<float*>(&position);
    if (ImGui::InputFloat3("Position", vecp, "%.3f")) {
        scene.addToColliderUpdateSet(selectedEntity);
    }

    float* rotp = reinterpret_cast<float*>(&rotation);
    if (ImGui::InputFloat4("Rotation", rotp, "%.3f")) {
        rotation = rotation;

        scene.addToColliderUpdateSet(selectedEntity);
    }

    float* scalep = reinterpret_cast<float*>(&scale);
    if (ImGui::InputFloat3("Scale", scalep, "%.3f")) {
        scene.addToColliderUpdateSet(selectedEntity);
    }

    ImGui::PopID();
    ImGui::PopItemWidth();

    endGroupPanel();
}

void EditorGui::showModel(Scene & /*scene*/, Model const & model) {
    static bool open = false;
    static double errorDeadline = 0.0;
    beginGroupPanel("Model");

    ImGui::Text("Name: %s", model.getName());

    if(ImGui::Button("load")) {
        open = true;
        errorDeadline = 0.0;
    }
    if (open) {
        ImGui::OpenPopup("Load model");

        if (file_dialog.showFileDialog("Load model", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(1400, 700))) {
            // std::cout << file_dialog.selected_fn << std::endl;      // The name of the selected file or directory in case of Select Directory dialog mode
            // std::cout << file_dialog.selected_path << std::endl;    // The absolute path to the selected file
            errorDeadline =  ImGui::GetTime() + 5.0;
        }
        open = ImGui::IsPopupOpen(ImGui::GetID("Load model"), 0);
    }
    
    if (ImGui::GetTime() < errorDeadline) {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s", "Failed to open models");
    }

    char buf[256] = {0};
    strcpy(buf, model.getPath());

    ImGui::InputText("Path", buf, 256, ImGuiInputTextFlags_ReadOnly);

    endGroupPanel();
}


void EditorGui::beginGroupPanel(const char* name, const ImVec2& size) {
    ImGui::BeginGroup();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f)
        effectiveSize.x = ImGui::GetContentRegionAvailWidth();
    else
        effectiveSize.x = size.x;
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    ImGui::PopStyleVar(2);

    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->Size.x                  -= frameHeight;

    ImGui::PushItemWidth(effectiveSize.x - frameHeight);
}

void EditorGui::endGroupPanel() {
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImGui::GetWindowDrawList()->AddRect(
        itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f),
        ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
        halfFrame.x);

    ImGui::PopStyleVar(2);

    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->Size.x                  += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}
