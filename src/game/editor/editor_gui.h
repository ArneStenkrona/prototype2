#ifndef PRT_EDITOR_GUI_H
#define PRT_EDITOR_GUI_H

#include "src/system/input/input.h"

#include "src/container/vector.h"

#include "src/game/scene/scene.h"

// TODO: move to separate source files

struct EditorUpdate {
    EntityID selectedEntity;
};

class EditorGui {
public:
    EditorGui(GLFWwindow * window, float width, float height);

    void update(Input & input, int width, int height, float deltaTime, Scene & scene, EditorUpdate const & updatePackage);
private:
    // TODO: set this in a dynamic, cross-platform manner 
    float dpiScaleFactor = 2.0f;

    EntityID selectedEntity = -1;

    void init(float width, float height);

    void updateInput(Input & input, int width, int height, float deltaTime);
    void newFrame(Scene & scene);
    void buildEditor(Scene & scene);
    void entityList(Entities & entities);
    void entityInfo(Entities & entities);
};

#endif
