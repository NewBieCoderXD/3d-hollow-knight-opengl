#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class HealthBar {
public:
  HealthBar(float width, float height, glm::vec2 position, glm::vec3 color)
      : width(width), height(height), position(position), color(color) {
    initQuad();
  }

  void setHealth(float current, float max) {
    currentHealth = current;
    maxHealth = max;
  }

  void setColor(const glm::vec3 &c) { color = c; }
  void setPosition(const glm::vec2 &pos) { position = pos; }
  void setSize(float w, float h) {
    width = w;
    height = h;
  }

  void draw(const glm::mat4 &projection, GLuint shaderID) {
    glUseProgram(shaderID);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position, 0.0f));
    float healthPercent = glm::clamp(currentHealth / maxHealth, 0.0f, 1.0f);
    model = glm::scale(model, glm::vec3(width * healthPercent, height, 1.0f));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint colorLoc = glGetUniformLocation(shaderID, "color");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(colorLoc, 1, &color[0]);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

private:
  GLuint VAO = 0, VBO = 0, EBO = 0;
  float width, height;
  glm::vec2 position;
  glm::vec3 color;
  float currentHealth = 100.0f;
  float maxHealth = 100.0f;

  void initQuad() {
    float vertices[] = {
        0.0f, 0.0f, // bottom-left
        1.0f, 0.0f, // bottom-right
        1.0f, 1.0f, // top-right
        0.0f, 1.0f  // top-left
    };
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);

    glBindVertexArray(0);
  }
};
