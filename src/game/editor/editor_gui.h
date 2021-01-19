#ifndef PRT_EDITOR_GUI_H
#define PRT_EDITOR_GUI_H

#include "src/system/input/input.h"

class EditorGui {
public:
    EditorGui() {}

    void update(Input & input, int w, int h, float deltaTime);
private:
    void updateInput(Input & input, int w, int h, float deltaTime);
    void newFrame();
};

#endif
