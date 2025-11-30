#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // Required for glm::value_ptr
#include <iostream>
#include <string>

#include "stb_image.h"

// --- GroundPlane Class Definition ---

class GroundPlane {
public:
  // Constructor: Takes the path to the ground texture image file, scale, and
  // tile factors
  GroundPlane(const std::string &texturePath, float scale, float tile);
  ~GroundPlane();

  // The primary drawing function
  void Draw(unsigned int shaderID, const glm::mat4 &view,
            const glm::mat4 &projection);

  // Setters for dynamic properties
  void SetScale(float scale) { this->scaleFactor = scale; }
  void SetTileFactor(float tile) { this->tileFactor = tile; }
  void SetFog(float min, float max, const glm::vec3 &color);

private:
  unsigned int VAO, VBO;
  unsigned int textureID;

  float scaleFactor;
  float tileFactor;

  // Helper functions
  void setupMesh();
  unsigned int loadTexture(const std::string &path);
};

// --- GroundPlane Implementation (Included in the same file) ---

GroundPlane::GroundPlane(const std::string &texturePath, float scale,
                         float tile)
    : scaleFactor(scale), tileFactor(tile) {
  setupMesh();
  textureID = loadTexture(texturePath);
}

GroundPlane::~GroundPlane() {
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteTextures(1, &textureID);
}

void GroundPlane::setupMesh() {
  // Defines a 2x2 quad in the XZ plane (Y=0)
  float vertices[] = {
      // Coords (x, y, z)    // Tex Coords (s, t)
      -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
      1.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Bottom-right
      -1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Top-left
      1.0f,  0.0f, 1.0f,  1.0f, 0.0f  // Top-right
  };

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute (layout location 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Texture coordinate attribute (layout location 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

unsigned int GroundPlane::loadTexture(const std::string &path) {
  unsigned int texture;
  glGenTextures(1, &texture);

  int width, height, nrChannels;
  unsigned char *data =
      stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

  if (data) {
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    if (nrChannels == 1)
      format = GL_RED;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Crucial: Set texture wrapping to GL_REPEAT for tiling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  } else {
    std::cerr << "Texture failed to load at path: " << path << std::endl;
    return 0; // Return 0 on failure
  }
  stbi_image_free(data);
  return texture;
}

void GroundPlane::Draw(unsigned int shaderID, const glm::mat4 &view,
                       const glm::mat4 &projection) {
  glUseProgram(shaderID);

  // Use an identity matrix for the model since the ground is centered
  glm::mat4 model = glm::mat4(1.0f);

  // Set Transformation Uniforms (Assuming the shader has these uniform names)
  glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE,
                     glm::value_ptr(model));
  glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE,
                     glm::value_ptr(view));
  glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE,
                     glm::value_ptr(projection));

  // Set Custom Ground Uniforms
  glUniform1f(glGetUniformLocation(shaderID, "scaleFactor"), scaleFactor);
  glUniform1f(glGetUniformLocation(shaderID, "tileFactor"), tileFactor);

  // Bind Texture (Set sampler to use texture unit 0)
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glUniform1i(glGetUniformLocation(shaderID, "groundTexture"), 0);

  // Draw the mesh
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}