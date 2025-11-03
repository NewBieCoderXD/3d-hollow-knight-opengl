#pragma once
#include "learnopengl/shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

class DebugBox {
public:
  DebugBox(const glm::vec3 &minCorner, const glm::vec3 &maxCorner)
      : color(1.0f, 0.0f, 0.0f), visible(false) {
    initBox(minCorner, maxCorner);
  }

  // Draw the box using a shader supplied by the caller.
  // The shader should already be bound and have view/projection set.
  void draw(const glm::mat4 &model, Shader &shaderProgram) const {
    if (!visible)
      return;

    // Set uniforms the box controls (only model + color)
    shaderProgram.setMat4("model", model);

    GLint colorLoc = glGetUniformLocation(shaderProgram.ID, "color");
    if (colorLoc != -1)
      glUniform3fv(colorLoc, 1, &color[0]);

    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
  }

  void setColor(const glm::vec3 &c) { color = c; }
  void setVisible(bool v) { visible = v; }
  bool isVisible() const { return visible; }

private:
  GLuint VAO = 0, VBO = 0, EBO = 0;
  glm::vec3 color;
  bool visible;

  void initBox(const glm::vec3 &minCorner, const glm::vec3 &maxCorner) {
    float vertices[] = {// 8 corners of bounding box
                        minCorner.x, minCorner.y, minCorner.z, maxCorner.x,
                        minCorner.y, minCorner.z, maxCorner.x, maxCorner.y,
                        minCorner.z, minCorner.x, maxCorner.y, minCorner.z,
                        minCorner.x, minCorner.y, maxCorner.z, maxCorner.x,
                        minCorner.y, maxCorner.z, maxCorner.x, maxCorner.y,
                        maxCorner.z, minCorner.x, maxCorner.y, maxCorner.z};

    unsigned int indices[] = {// front
                              0, 1, 1, 2, 2, 3, 3, 0,
                              // back
                              4, 5, 5, 6, 6, 7, 7, 4,
                              // sides
                              0, 4, 1, 5, 2, 6, 3, 7};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);

    glBindVertexArray(0);
  }
};
