#ifndef MODEL_H
#define MODEL_H

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include "learnopengl/bone.h"
#include <assimp/anim.h>
#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <fstream>
#include <glm/gtx/string_cast.hpp> // For glm::to_string
#include <iostream>
#include <learnopengl/animdata.h>
#include <learnopengl/assimp_glm_helpers.h>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct MeshAnimationChannel {
  aiNode *node;
  aiNodeAnim *channel;
  glm::mat4 mTransform;
};

class Model {
public:
  // model data
  vector<Texture>
      textures_loaded; // stores all the textures loaded so far, optimization to
                       // make sure textures aren't loaded more than once.
  vector<Mesh> meshes;
  string directory;
  bool gammaCorrection;
  std::unordered_map<unsigned int, MeshAnimationChannel> meshAnimationCache;

  // constructor, expects a filepath to a 3D model.
  Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
    loadModel(path);
  }

  Model(const aiScene *scene, const std::string &directory, bool gamma = false)
      : gammaCorrection(gamma), directory(directory) {
    if (!scene || !scene->mRootNode) {
      std::cerr << "ERROR::MODEL::INVALID_SCENE_POINTER\n";
      return;
    }
    processNode(scene->mRootNode, scene);

    if (scene->mNumAnimations > 0) {
      aiAnimation *animation =
          scene->mAnimations[0]; // pick first animation for now
      BuildMeshAnimationCache(scene, animation);
    }
  }

  aiNode *FindNodeForMesh(aiNode *node, unsigned int meshIndex) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
      if (node->mMeshes[i] == meshIndex)
        return node;
    for (unsigned int c = 0; c < node->mNumChildren; c++) {
      aiNode *found = FindNodeForMesh(node->mChildren[c], meshIndex);
      if (found)
        return found;
    }
    return nullptr;
  }

  void BuildMeshAnimationCache(const aiScene *scene,
                               const aiAnimation *animation) {
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes;
         ++meshIndex) {
      aiNode *node = FindNodeForMesh(scene->mRootNode, meshIndex);
      if (!node)
        continue;

      aiNodeAnim *channel = nullptr;
      for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        if (animation->mChannels[i]->mNodeName == node->mName) {
          channel = animation->mChannels[i];
          break;
        }
      }

      meshAnimationCache[meshIndex] = {
          node, channel,
          AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation)};
    }
  }

  // draws the model, and thus all its meshes
  void Draw(glm::mat4 objectModel, Shader &shader, float time) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
      MeshAnimationChannel found = meshAnimationCache[i];
      glm::mat4 transform = found.mTransform;
      glm::mat4 animTransform = glm::mat4(1.0f);
      if (found.channel != nullptr) {
        glm::vec3 pos = AssimpGLMHelpers::LerpPosition(found.channel, time);
        glm::quat rot = AssimpGLMHelpers::SlerpRotation(found.channel, time);
        glm::vec3 scale = AssimpGLMHelpers::LerpScale(found.channel, time);

        // float oldY = pos.y;
        // pos.y = pos.z;
        // pos.z = oldY;

        // float oldY = rot.y;
        // rot.y = rot.z;
        // rot.z = oldY;

        // glm::quat correction =
        //     glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        // rot = correction * rot;

        animTransform = glm::translate(animTransform, pos);
        glm::mat4 rotMat = glm::toMat4(rot);
        animTransform *= glm::toMat4(rot);
        animTransform = glm::scale(animTransform, scale);
        transform = animTransform;
      }
      shader.use();
      shader.setMat4("model", objectModel * transform);

      meshes[i].Draw(shader);
    }
  }

  auto &GetBoneInfoMap() { return m_BoneInfoMap; }
  int &GetBoneCount() { return m_BoneCounter; }

private:
  std::map<string, BoneInfo> m_BoneInfoMap;
  int m_BoneCounter = 0;

  // loads a model with supported ASSIMP extensions from file and stores the
  // resulting meshes in the meshes vector.
  void loadModel(string const &path) {
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                  aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) // if is Not Zero
    {
      cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
      return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
  }

  // processes a node in a recursive fashion. Processes each individual mesh
  // located at the node and repeats this process on its children nodes (if
  // any).
  void processNode(aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      // the node object only contains indices to index the actual objects in
      // the scene. the scene contains all the data, node is just to keep stuff
      // organized (like relations between nodes).
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively
    // process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene);
    }
  }

  void SetVertexBoneDataToDefault(Vertex &vertex) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
      vertex.m_BoneIDs[i] = -1;
      vertex.m_Weights[i] = 0.0f;
    }
  }

  Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      SetVertexBoneDataToDefault(vertex);
      vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
      vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);

      if (mesh->mTextureCoords[0]) {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.TexCoords = vec;
      } else
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);

      vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    vector<Texture> diffuseMaps = loadMaterialTextures(
        *scene, material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    vector<Texture> specularMaps = loadMaterialTextures(
        *scene, material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    std::vector<Texture> normalMaps = loadMaterialTextures(
        *scene, material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    std::vector<Texture> heightMaps = loadMaterialTextures(
        *scene, material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    ExtractBoneWeightForVertices(vertices, mesh, scene);

    return Mesh(vertices, indices, textures);
  }

  void SetVertexBoneData(Vertex &vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
      if (vertex.m_BoneIDs[i] < 0) {
        vertex.m_Weights[i] = weight;
        vertex.m_BoneIDs[i] = boneID;
        break;
      }
    }
  }

  void ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh,
                                    const aiScene *scene) {
    auto &boneInfoMap = m_BoneInfoMap;
    int &boneCount = m_BoneCounter;

    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
      int boneID = -1;
      std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
      if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
        BoneInfo newBoneInfo;
        newBoneInfo.id = boneCount;
        newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(
            mesh->mBones[boneIndex]->mOffsetMatrix);
        boneInfoMap[boneName] = newBoneInfo;
        boneID = boneCount;
        boneCount++;
      } else {
        boneID = boneInfoMap[boneName].id;
      }
      assert(boneID != -1);
      auto weights = mesh->mBones[boneIndex]->mWeights;
      int numWeights = mesh->mBones[boneIndex]->mNumWeights;

      for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
        int vertexId = weights[weightIndex].mVertexId;
        float weight = weights[weightIndex].mWeight;
        assert(vertexId <= vertices.size());
        SetVertexBoneData(vertices[vertexId], boneID, weight);
      }
    }
  }

  unsigned int TextureFromFile(const char *path, const string &directory,
                               bool gamma = false) {
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data =
        stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
      GLenum format;
      if (nrComponents == 1)
        format = GL_RED;
      else if (nrComponents == 3)
        format = GL_RGB;
      else if (nrComponents == 4)
        format = GL_RGBA;

      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      stbi_image_free(data);
    } else {
      std::cout << "Texture failed to load at path: " << path << std::endl;
      stbi_image_free(data);
    }

    return textureID;
  }

  // checks all material textures of a given type and loads the textures if
  // they're not loaded yet. the required info is returned as a Texture struct.
  vector<Texture> loadMaterialTextures(const aiScene &scene, aiMaterial *mat,
                                       aiTextureType type, string typeName) {
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
      aiString str;
      if (mat->GetTexture(type, i, &str) == AI_SUCCESS) {
        if (str.C_Str()[0] == '*') {
          // embedded texture
          int texIndex = atoi(str.C_Str() + 1);
          aiTexture *tex = scene.mTextures[texIndex];

          int width, height, nrComponents;
          unsigned char *data = nullptr;

          if (tex->mHeight == 0) {
            // Compressed texture (PNG/JPG in memory)
            stbi_set_flip_vertically_on_load(true);
            data =
                stbi_load_from_memory((unsigned char *)tex->pcData, tex->mWidth,
                                      &width, &height, &nrComponents, 0);
          } else {
            // Raw RGBA
            width = tex->mWidth;
            height = tex->mHeight;
            nrComponents = 4;
            data = (unsigned char *)tex->pcData;
          }

          if (!data) {
            std::cerr << "Failed to load embedded texture: " << str.C_Str()
                      << std::endl;
            return {};
          }

          unsigned int textureID;
          glGenTextures(1, &textureID);

          GLenum format = (nrComponents == 1)   ? GL_RED
                          : (nrComponents == 3) ? GL_RGB
                                                : GL_RGBA;

          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                       GL_UNSIGNED_BYTE, data);
          glGenerateMipmap(GL_TEXTURE_2D);

          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          Texture texture;
          texture.id = textureID;
          texture.type = typeName;
          texture.path = str.C_Str();
          textures.push_back(texture);
          textures_loaded.push_back(texture);

          if (tex->mHeight == 0)
            stbi_image_free(data); // free only if we used stbi
          continue;
        }
        // check if texture was loaded before and if so, continue to next
        // iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
          if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
            textures.push_back(textures_loaded[j]);
            skip = true; // a texture with the same filepath has already been
                         // loaded, continue to next one. (optimization)
            break;
          }
        }
        if (!skip) { // if texture hasn't been loaded already, load it
          Texture texture;
          texture.id = TextureFromFile(str.C_Str(), this->directory);
          texture.type = typeName;
          texture.path = str.C_Str();
          textures.push_back(texture);
          textures_loaded.push_back(
              texture); // store it as texture loaded for entire model, to
                        // ensure we won't unnecessary load duplicate textures.
        }
      }
    }
    return textures;
  }
};

#endif
