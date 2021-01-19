#ifndef PRT_EDITOR_GUI_H
#define PRT_EDITOR_GUI_H

#include "src/system/input/input.h"

class EditorGui {
public:
    EditorGui(float width, float height);

    void update(Input & input, int width, int height, float deltaTime);
private:
    // bool open = true;
    // TODO: set this in a dynamic, cross-platform manner 
    float dpiScaleFactor = 2.0f;

    void init(float width, float height);

    void updateInput(Input & input, int width, int height, float deltaTime);
    void newFrame();
    void buildEditor();
};

#endif
