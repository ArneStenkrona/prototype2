#ifndef PRT_CAMERA_H
#define PRT_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include "src/system/input/input.h"

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // Constructor with vectors
    Camera(Input& input, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);
    // Constructor with scalar values
    Camera(Input& input, float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    void setTarget(glm::vec3 target);
    void update(float deltaTime);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    inline glm::mat4 getViewMatrix() const { return glm::lookAt(_position, _position + _front, _up); }
    glm::mat4 getProjectionMatrix(float width, float height, float near, float far) const;
    
    inline float getFOV() const { return _fieldOfView; }
    

    inline glm::vec3 getPosition() const { return _position; }
    inline glm::vec3 getFront() const { return _front; }
    inline glm::vec3 getUp() const { return _up; }
    inline glm::vec3 getRight() const { return _right; }
    inline glm::vec3 getWorldUp() const { return _worldUp; }

private:
    // Camera Attributes
    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;
    // Euler Angles
    float _yaw;
    float _pitch;
    // Camera options
    float _movementSpeed;
    float _mouseSensitivity;
    float _fieldOfView;

    Input& _input;

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyboard(float deltaTime);

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement();

    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};

#endif