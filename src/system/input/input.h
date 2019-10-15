#ifndef PRT_INPUT_H
#define PRT_INPUT_H
#include <GLFW/glfw3.h>


class Input {
public:
    Input(GLFWwindow* window);

    int getKey(int keyCode); 

    void getCursorPos(double& xpos, double& ypos);
    void getCursorDelta(double& dx, double& dy);
private:
    GLFWwindow* _window;

    double _lastCursorX;
    double _lastCursorY;
    double _dx;
    double _dy;

    void update();

    friend class Game;
};


#endif