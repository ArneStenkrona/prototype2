#ifndef PRT_EDITOR_GUI_H
#define PRT_EDITOR_GUI_H

#include "src/system/input/input.h"

#include "src/container/vector.h"

#include "src/game/scene/scene.h"

#include "imgui-filebrowser/ImGuiFileBrowser.h"

// TODO: move to separate source files

struct EditorUpdate {
    EntityID selectedEntity = -1;
};

class EditorGui {
public:
    EditorGui(GLFWwindow * window, float width, float height);

    void update(Input & input, int width, int height, float deltaTime, Scene & scene, EditorUpdate const & updatePackage);
private:
    // TODO: set this in a dynamic, cross-platform manner 
    float dpiScaleFactor = 2.0f;

    EntityID selectedEntity = -1;

    imgui_addons::ImGuiFileBrowser file_dialog;

    void init(float width, float height);

    void updateInput(Input & input, int width, int height, float deltaTime);
    void newFrame(Scene & scene);
    void buildEditor(Scene & scene);
    void entityList(Scene & scene, Entities & entities);
    void entityInfo(Scene & scene, Entities & entities);
    void showTransform(Scene & scene, Transform & transform);
    void showModel(Scene & scene, Model const & model);
    void showCollider(Scene & scene, ColliderTag const & tag);
    void showCharacter(Scene & scene, CharacterID const & id);

    // Thanks thedmd: https://github.com/ocornut/imgui/issues/1496#issuecomment-569892444
    static void beginGroupPanel(const char* name, const ImVec2& size = ImVec2(-1.0f, -1.0f));
    static void endGroupPanel();
};

#endif
