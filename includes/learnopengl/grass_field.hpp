#pragma once
#include "learnopengl/shader.h"
#include <chrono>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <vector>

class GrassField {
public:
  GrassField(int gridX, int gridZ, float spacing) {
    std::vector<float> verts;
    std::vector<unsigned int> idx;
    buildBladeMesh(verts, idx);
    indexCount = (GLsizei)idx.size();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int),
                 idx.data(), GL_STATIC_DRAW);

    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
    //                       (void *)0);

    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
    //                       (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);

    // texcoord = 2 floats
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));

    buildInstances(gridX, gridZ, spacing);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void *)0);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
  }

  void draw(Shader shader, float time, glm::mat4 view, glm::mat4 projection) {
    shader.use();
    shader.setFloat("uTime", time);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0,
                            instanceCount);
    glBindVertexArray(0);
  }

private:
  // ----------------------------------------------------------------------
  // Internal Data
  // ----------------------------------------------------------------------
  GLuint vao = 0, vbo = 0, ebo = 0, instanceVBO = 0;
  GLsizei indexCount = 0;
  GLsizei instanceCount = 0;

  struct InstanceData {
    float x, y, z;
    float phase;
  };

  // ----------------------------------------------------------------------
  // Build blade mesh (simple quad)
  // ----------------------------------------------------------------------
  void buildBladeMesh(std::vector<float> &verts,
                      std::vector<unsigned int> &idx) {
    float h = 1.5f;
    float w = 0.25f;

    // 3D triangular grass blade
    // Format: x, y, z, u, v

    verts = {// Bottom ring (triangle)
             -w, 0.0f, 0.0f, 0.0f, 0.0f, w, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
             w, 0.5f, 0.0f,

             // Top ring (narrower, slightly forward for curvature)
             -w * 0.1f, h, w * 0.1f, 0.0f, 1.0f, w * 0.1f, h, w * 0.1f, 1.0f,
             1.0f, 0.0f, h, w * 0.3f, 0.5f, 1.0f};

    idx = {// Sides
           0, 1, 3, 1, 4, 3,

           1, 2, 4, 2, 5, 4,

           2, 0, 5, 0, 3, 5};
  }

  // ----------------------------------------------------------------------
  // Build instances
  // ----------------------------------------------------------------------
  void buildInstances(int gridX, int gridZ, float spacing) {
    std::vector<InstanceData> data;
    data.reserve(gridX * gridZ);

    float offX = -gridX * spacing * 0.5f;
    float offZ = -gridZ * spacing * 0.5f;

    std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count());
    std::uniform_real_distribution<float> jitter(-0.02f, 0.02f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 6.28318f);

    for (int x = 0; x < gridX; x++) {
      for (int z = 0; z < gridZ; z++) {
        float px = offX + x * spacing + jitter(rng);
        float pz = offZ + z * spacing + jitter(rng);
        data.push_back({px, 0.0f, pz, phaseDist(rng)});
      }
    }

    instanceCount = (GLsizei)data.size();

    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(InstanceData),
                 data.data(), GL_STATIC_DRAW);
  }
};
