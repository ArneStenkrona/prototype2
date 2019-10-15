#ifndef PRT_INPUT_H
#define PRT_INPUT_H
#include <GLFW/glfw3.h>

namespace input {

int getKey(int keyCode); 

void getCursorPos(double* xpos, double* ypos);

};

#endif