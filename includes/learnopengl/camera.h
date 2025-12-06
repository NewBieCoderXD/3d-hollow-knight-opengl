#ifndef CAMERA_H
#define CAMERA_H

#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/trigonometric.hpp"
#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

// Defines several possible options for camera movement. Used as abstraction to
// stay away from window-system specific input methods
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 60.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 100.0f;
const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the
// corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera {
public:
  // camera Attributes
  glm::vec3 Position;
  glm::vec3 Front;
  glm::vec3 Up;
  glm::vec3 LookAt;
  glm::vec3 Right;
  glm::vec3 WorldUp;
  // euler Angles

  glm::vec3 KnightFront;

  float Yaw;
  float Pitch;
  // camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;
  // glm::vec3 cameraOffset{-1.0f, 0.0f, 6.0f};
  float distance = 15.0;

  // calculates the front vector from the Camera's (updated) Euler Angles
  void UpdateCameraVectors() {
    float radYaw = glm::radians(Yaw);
    float radPitch = glm::radians(Pitch);

    float x = distance * cos(radPitch) * cos(radYaw);
    float y = distance * sin(radPitch);
    float z = distance * cos(radPitch) * sin(radYaw);

    glm::vec3 offset = glm::vec3(x, y, z);

    Position = LookAt + offset;
    Front = glm::normalize(LookAt - Position);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
  }

  // constructor with vectors
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
         float pitch = PITCH)
      : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
        MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    KnightFront = glm::vec3(0.0f, 0.0f, 1.0f); // initial knight facing +Z
    UpdateCameraVectors();
  }
  // constructor with scalar values
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
         float yaw, float pitch)
      : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
        MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    KnightFront = glm::vec3(0.0f, 0.0f, 1.0f); // initial knight facing +Z
    UpdateCameraVectors();
  }

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 GetViewMatrix() {
    return glm::lookAt(Position, LookAt, glm::vec3(0, 1, 0));
  }

  // processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
      Position += Front * velocity;
    if (direction == BACKWARD)
      Position -= Front * velocity;
    if (direction == LEFT)
      Position -= Right * velocity;
    if (direction == RIGHT)
      Position += Right * velocity;
  }

  // processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void ProcessMouseMovement(float xoffset, float yoffset,
                            GLboolean constrainPitch = true) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // Pitch = std::max(40.0f, std::min(75.0f, Pitch));

    // make sure that when pitch is out of bounds, screen doesn't get
    // flipped
    if (constrainPitch) {
      Pitch = std::max(0.0f, std::min(40.0f, Pitch));
      // if (Pitch < -89.0f)
      //   Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    UpdateCameraVectors();
  }

  // processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis
  void ProcessMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
      Zoom = 1.0f;
    if (Zoom > 45.0f)
      Zoom = 45.0f;
  }
};
#endif
