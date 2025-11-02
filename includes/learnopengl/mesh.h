#ifndef MESH_H
#define MESH_H

#include "assimp/aabb.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader.h>

#include <string>
#include <sys/types.h>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
  // position
  glm::vec3 Position;
  // normal
  glm::vec3 Normal;
  // texCoords
  glm::vec2 TexCoords;
  // tangent
  glm::vec3 Tangent;
  // bitangent
  glm::vec3 Bitangent;
  // bone indexes which will influence this vertex
  int m_BoneIDs[MAX_BONE_INFLUENCE];
  // weights from each bone
  float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
  unsigned int id;
  string type;
  string path;
};

struct Material {
  aiColor3D diffuse;
  aiColor3D specular;
  aiColor3D emissive;
  float shininess;
  float roughness;
  float metallic;
  float opacity;
  std::unordered_map<aiTextureType, Texture> textures;
};

struct AABB {
  glm::vec3 mMin;
  glm::vec3 mMax;
};

class Mesh {
public:
  // mesh Data
  vector<Vertex> vertices;
  vector<unsigned int> indices;
  vector<Texture> textures;
  unsigned int VAO;
  bool hasBones;
  string name;
  Material material;
  AABB mAABB;

  // constructor
  Mesh(vector<Vertex> vertices, vector<unsigned int> indices,
       vector<Texture> textures, string name, aiAABB mAiAABB, bool hasBones)
      : hasBones(hasBones) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->name = name;
    this->mAABB = {
        .mMin = glm::vec3(mAiAABB.mMin.x, mAiAABB.mMin.y, mAiAABB.mMin.z),
        .mMax = glm::vec3(mAiAABB.mMax.x, mAiAABB.mMax.y, mAiAABB.mMax.z)};
    // for (uint i = 0; i < this->vertices.size(); i++) {
    //   for (auto vertex : vertices) {
    //     std::cout << "x: " << vertex.Position.x << " y: " <<
    //     vertex.Position.y
    //               << " z: " << vertex.Position.z << std::endl;
    //     std::cout << ",\n";
    //   }
    //   std::cout << "\n";
    // }

    // now that we have all the required data, set the vertex buffers and its
    // attribute pointers.
    setupMesh();
  }

  // render the mesh
  void Draw(Shader &shader) {
    std::unordered_map<std::string, unsigned int> textureCount;

    for (unsigned int i = 0; i < textures.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i); // activate texture unit

      std::string name =
          textures[i].type; // e.g. "texture_diffuse", "texture_specular", etc.
      std::string uniformName;

      // Ensure each texture type has an incremented number suffix
      unsigned int count = ++textureCount[name];
      uniformName = name + std::to_string(count);
      // std::cout << "  " << uniformName << " id: " << textures[i].id
      //           << std::endl;

      // Send texture unit to shader
      glUniform1i(glGetUniformLocation(shader.ID, uniformName.c_str()), i);

      // Bind actual texture
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    for (unsigned int i = 0; i < textures.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, 0); // unbind texture
    }

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0); //
  }

private:
  // render data
  unsigned int VBO, EBO;

  // initializes all the buffer objects/arrays
  void setupMesh() {
    // create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // A great thing about structs is that their memory layout is sequential for
    // all its items. The effect is that we can simply pass a pointer to the
    // struct and it translates perfectly to a glm::vec3/2 array which again
    // translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Tangent));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Bitangent));
    // ids
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex),
                           (void *)offsetof(Vertex, m_BoneIDs));

    // weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, m_Weights));
    glBindVertexArray(0);
  }
};
#endif
