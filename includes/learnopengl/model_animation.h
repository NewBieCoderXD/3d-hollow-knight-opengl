#ifndef MODEL_H
#define MODEL_H

#include "assimp/vector3.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "learnopengl/bone.h"
#include "learnopengl/box.hpp"
#include <assimp/anim.h>
#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
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
class IAnimator {
public:
  virtual ~IAnimator() = default;
  virtual glm::mat4 GetGlobalNodeTransform(std::string) = 0;
};

// struct MeshAnimationChannel {
//   aiNode *node;
//   aiNodeAnim *channel;
//   glm::mat4 mTransform;
// };
static const std::vector<std::pair<aiTextureType, std::string>> textureTypes = {
    {aiTextureType_DIFFUSE, "texture_diffuse"},
    {aiTextureType_SPECULAR, "texture_specular"},
    {aiTextureType_AMBIENT, "texture_ambient"},
    {aiTextureType_EMISSIVE, "texture_emissive"},
    {aiTextureType_HEIGHT, "texture_height"},
    {aiTextureType_NORMALS, "texture_normal"},
    {aiTextureType_SHININESS, "texture_shininess"},
    {aiTextureType_OPACITY, "texture_opacity"},
    {aiTextureType_DISPLACEMENT, "texture_displacement"},
    {aiTextureType_LIGHTMAP, "texture_lightmap"},
    {aiTextureType_REFLECTION, "texture_reflection"},
    {aiTextureType_BASE_COLOR, "texture_base_color"},
    {aiTextureType_NORMAL_CAMERA, "texture_normal_camera"},
    {aiTextureType_EMISSION_COLOR, "texture_emission_color"},
    {aiTextureType_METALNESS, "texture_metalness"},
    {aiTextureType_DIFFUSE_ROUGHNESS, "texture_diffuse_roughness"},
    {aiTextureType_AMBIENT_OCCLUSION, "texture_ambient_occlusion"},
    {aiTextureType_UNKNOWN, "texture_unknown"}};

class Model {
public:
  std::vector<glm::mat4> meshNodeTransforms;
  // model data
  vector<Texture>
      textures_loaded; // stores all the textures loaded so far, optimization to
                       // make sure textures aren't loaded more than once.
  vector<Mesh> meshes;
  string directory;
  bool gammaCorrection;
  string weaponMesh = "";
  glm::vec3 weaponSize = glm::vec3(0.0f);
  glm::vec3 weaponPos = glm::vec3(0.0f);
  std::unique_ptr<DebugBox> weaponHitbox;
  std::string currentAnim;

  unsigned int weaponMeshIndex = 0;

  // constructor, expects a filepath to a 3D model.
  Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
    loadModel(path);
  }

  Model(const aiScene *scene, const std::string &directory,
        std::string weaponMesh, bool gamma = false)
      : directory(directory), gammaCorrection(gamma) {
    if (!scene || !scene->mRootNode) {
      std::cerr << "ERROR::MODEL::INVALID_SCENE_POINTER\n";
      return;
    };

    this->weaponMesh = weaponMesh;

    processNode(scene->mRootNode, scene);
    std::cout << "Number of meshes: " << meshes.size() << std::endl;

    // for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
    //   aiAnimation *animation = scene->mAnimations[0];
    //   std::vector<MeshAnimationChannel> meshAnimation(meshes.size());
    //   // BuildMeshAnimationCache(scene, animation, meshAnimation);
    // }
  }

  void computeMeshSize(aiMesh *mesh, glm::mat4 nodeTransform, aiVector3D &min,
                       aiVector3D &max) const {
    aiVector3D aabbMin = mesh->mAABB.mMin;
    aiVector3D aabbMax = mesh->mAABB.mMax;

    for (int i = 0; i < 8; ++i) {
      glm::vec3 corner((i & 1) ? aabbMax.x : aabbMin.x,
                       (i & 2) ? aabbMax.y : aabbMin.y,
                       (i & 4) ? aabbMax.z : aabbMin.z);

      // Apply node transform
      glm::vec4 transformed = nodeTransform * glm::vec4(corner, 1.0f);

      // Update min/max
      min.x = std::min(min.x, transformed.x);
      min.y = std::min(min.y, transformed.y);
      min.z = std::min(min.z, transformed.z);

      max.x = std::max(max.x, transformed.x);
      max.y = std::max(max.y, transformed.y);
      max.z = std::max(max.z, transformed.z);
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

  glm::mat4 GetBoneOffsetMatrix(const std::string &name) const {
    if (m_BoneInfoMap.count(name))
      return m_BoneInfoMap.at(name).offset;
    return glm::mat4(1.0f); // Identity if not found
  }

  // draws the model, and thus all its meshes
  void Draw(glm::mat4 objectModel, IAnimator &animator, Shader &shader,
            Shader &hitboxShader, bool showHitbox) {
    // bool hasBones = false;
    for (unsigned int i = 0; i < meshes.size(); i++) {
      Mesh mesh = meshes[i];
      // for (auto &vertex : meshes[i].vertices) {
      //   for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
      //     if (vertex.m_BoneIDs[j] >= 0) {
      //       hasBones = true;
      //       break;
      //     }
      //   }
      //   if (hasBones)
      //     break;
      // }
      glm::mat4 animatedNodeTransform =
          animator.GetGlobalNodeTransform(mesh.name);
      glm::mat4 finalTransform = objectModel * animatedNodeTransform;
      // if (!hasBones) {
      //   glm::mat4 transform = meshNodeTransforms[i];
      //   std::optional<glm::mat4> animTrans =
      //       animator.getMeshTransform(i, timeInTicks);
      //   if (animTrans.has_value()) {
      //     transform = *animTrans;
      //   }
      //   finalTransform *= transform;
      // }
      shader.setMat4("model", finalTransform);

      mesh.Draw(shader);

      if (i == weaponMeshIndex && showHitbox && weaponHitbox != nullptr) {
        weaponPos = glm::vec3(finalTransform[3]);
        weaponHitbox->setVisible(true);
        weaponHitbox->draw(glm::translate(glm::mat4(1.0f), weaponPos),
                           hitboxShader);
      }
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
  void processNode(aiNode *node, const aiScene *scene,
                   const glm::mat4 &parentTransform = glm::mat4(1.0f)) {
    glm::mat4 nodeTransform =
        parentTransform *
        AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      // the node object only contains indices to index the actual objects in
      // the scene. the scene contains all the data, node is just to keep stuff
      // organized (like relations between nodes).
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

      // std::cout << "gg" << mesh->mName.C_Str() << weaponMesh << std::endl;
      if (mesh->mName.C_Str() == weaponMesh) {
        aiVector3D weaponMin(FLT_MAX, FLT_MAX, FLT_MAX);
        aiVector3D weaponMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        computeMeshSize(mesh, nodeTransform, weaponMin, weaponMax);
        weaponSize =
            glm::vec3(weaponMax.x - weaponMin.x, weaponMax.y - weaponMin.y,
                      weaponMax.z - weaponMin.z);

        std::cout << "weapon: " << this->weaponMesh
                  << " size: " << glm::to_string(weaponSize) << std::endl;

        weaponHitbox = std::make_unique<DebugBox>(
            glm::vec3(weaponMin.x, weaponMin.y, weaponMin.z),
            glm::vec3(weaponMax.x, weaponMax.y, weaponMax.z));

        weaponMeshIndex = meshes.size();
      }

      meshes.push_back(processMesh(mesh, scene));
      meshNodeTransforms.push_back(nodeTransform);
    }
    // after we've processed all of the meshes (if any) we then recursively
    // process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene, nodeTransform);
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
    aiMaterial *aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
    Material material;

    // --- Load material colors ---
    aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, material.diffuse);
    aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, material.specular);
    aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, material.emissive);

    aiMaterial->Get(AI_MATKEY_SHININESS, material.shininess);
    aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughness);
    aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, material.metallic);
    aiMaterial->Get(AI_MATKEY_OPACITY, material.opacity);

    // vector<Texture> diffuseMaps = loadMaterialTextures(
    //     *scene, material, aiTextureType_DIFFUSE, "texture_diffuse");
    // textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // vector<Texture> specularMaps = loadMaterialTextures(
    //     *scene, material, aiTextureType_SPECULAR, "texture_specular");
    // textures.insert(textures.end(), specularMaps.begin(),
    // specularMaps.end()); std::vector<Texture> normalMaps =
    // loadMaterialTextures(
    //     *scene, material, aiTextureType_HEIGHT, "texture_normal");
    // textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // std::vector<Texture> heightMaps = loadMaterialTextures(
    //     *scene, material, aiTextureType_AMBIENT, "texture_height");
    // textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    for (auto &texType : textureTypes) {
      std::vector<Texture> loaded = loadMaterialTextures(
          *scene, aiMaterial, texType.first, texType.second);
      if (!loaded.empty()) {
        std::cout << "Loaded " << loaded.size() << " " << texType.second
                  << "(s)\n";
      }
      textures.insert(textures.end(), loaded.begin(), loaded.end());
    }

    std::cout << "Material " << mesh->mMaterialIndex << " has "
              << textures.size() << " textures:\n";
    for (const auto &tex : textures) {
      std::cout << "  [" << tex.type << "] " << tex.path << " id: " << tex.id
                << "\n";
    }

    ExtractBoneWeightForVertices(vertices, mesh, scene);

    return Mesh(vertices, indices, textures, mesh->mName.C_Str(), mesh->mAABB);
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

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
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
        long unsigned int vertexId = weights[weightIndex].mVertexId;
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
        std::cout << "Found texture: " << str.C_Str() << std::endl;
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

          // std::cout << "id " << textureID
          //           << " First pixel RGBA: " << (int)tex->pcData->r << " "
          //           << (int)tex->pcData->g << " " << (int)tex->pcData->b << "
          //           "
          //           << (int)tex->pcData->a << std::endl;

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
