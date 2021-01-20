#ifndef PRT_EDITOR_GUI_H
#define PRT_EDITOR_GUI_H

#include "src/system/input/input.h"

#include "src/container/vector.h"

#include "src/game/scene/scene.h"

// TODO: move to separate source files

class EditorGui {
public:
    EditorGui(float width, float height);

    void update(Input & input, int width, int height, float deltaTime, Scene & scene);
private:
    // TODO: set this in a dynamic, cross-platform manner 
    float dpiScaleFactor = 2.0f;

    void init(float width, float height);

    void updateInput(Input & input, int width, int height, float deltaTime);
    void newFrame(Scene & scene);
    void buildEditor(Scene & scene);
    void entityList(Entities & entities);
};

#endif
