#ifndef PRT_INPUT_H
#define PRT_INPUT_H
#include <GLFW/glfw3.h>


class Input {
public:
    Input();

    int getKey(int keyCode); 
    int getMouseButton(int mouseButton);

    void getCursorPos(double& xpos, double& ypos);
    void getCursorDelta(double& dx, double& dy);

    inline GLFWwindow* getWindow() const { return _window; }
private:
    GLFWwindow* _window;
    double scaleX, scaleY;

    double _lastCursorX;
    double _lastCursorY;
    double _dx;
    double _dy;

    void init(GLFWwindow* window);
    void update();
    friend class Game;
};


#endif