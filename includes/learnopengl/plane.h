#pragma once
#include "learnopengl/shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PlaneWithControls {
public:
  unsigned int VAO, VBO;
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec3 rotation; // Euler angles

  PlaneWithControls(float size = 10.0f, unsigned int tex = 0)
      : position(0.0f), scale(glm::vec3(size)), rotation(0.0f) {
    float vertices[] = {
        // positions (X, Y, Z)
        1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f, -1.0f,

        1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f,  0.0f, -1.0f};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // only position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
  }

  void Draw(Shader &shader) {
    shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    shader.setMat4("model", model);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }
};
