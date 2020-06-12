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

    /**
     * Retrieve world-space coordinates of camera
     * corners
     * @param topleft top left corner, returned by reference
     * @param topleft top right corner, returned by reference
     * @param topleft bottom left corner, returned by reference
     * @param topleft bottom right corner, returned by reference
     */
    void getCameraCorners(glm::vec3 & topleft,
                          glm::vec3 & topright,
                          glm::vec3 & bottomleft,
                          glm::vec3 & bottomright);

    void setTarget(glm::vec3 target);
    void update(float deltaTime);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    inline glm::mat4 getViewMatrix() const { return glm::lookAt(m_position, m_position + m_front, m_up); }
    void setProjection(float width, float height, float near, float far);
    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getProjectionMatrix(float near, float far) const;
    
    inline float getFOV() const { return m_fieldOfView; }

    inline float getTargetDistance() const { return m_targetDistance; }
    inline void setTargetDistance(float distance) { m_targetDistance = distance; }
    

    inline glm::vec3 const & getPosition() const { return m_position; }
    inline glm::vec3 const & getFront() const { return m_front; }
    inline glm::vec3 const & getUp() const { return m_up; }
    inline glm::vec3 const & getRight() const { return m_right; }
    inline glm::vec3 const & getWorldUp() const { return m_worldUp; }

private:
    // Camera Attributes
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;
    // Euler Angles
    float m_yaw;
    float m_pitch;
    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_fieldOfView;
    // Projection
    float m_width;
    float m_height;
    float m_nearPlane;
    float m_farPlane;

    // Target attributes
    float m_targetDistance;

    Input& m_input;

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyboard(float deltaTime);

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement();

    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};

#endif