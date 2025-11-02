#pragma once

#include "learnopengl/model_animation.h"
#include <assimp/anim.h>
#include <assimp/scene.h>
#include <functional>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <learnopengl/animdata.h>
#include <learnopengl/bone.h>
// #include <learnopengl/model_animation.h>
#include <map>
#include <vector>

struct AssimpNodeData {
  glm::mat4 localTransformation;
  std::string name;
  int childrenCount;
  std::vector<AssimpNodeData> children;
};

struct MeshAnimationChannel {
  aiNode *node;
  aiNodeAnim *channel;
  glm::mat4 mTransform;
};

class Animation {
public:
  float m_Duration;
  int m_TicksPerSecond;
  std::string name;
  std::vector<MeshAnimationChannel> meshToChannel;
  glm::mat4 m_GlobalInverseTransform;
  aiAnimation *animation;
  Animation() = default;

  // Animation(const std::string &animationPath, Model *model) {
  //   Assimp::Importer importer;
  //   const aiScene *scene =
  //       importer.ReadFile(animationPath, aiProcess_Triangulate);
  //   assert(scene && scene->mRootNode);
  //   animation = scene->mAnimations[0];
  //   m_Duration = animation->mDuration;
  //   m_TicksPerSecond = animation->mTicksPerSecond;
  //   aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
  //   globalTransformation = globalTransformation.Inverse();
  //   ReadHierarchyData(m_RootNode, scene->mRootNode);
  //   ReadMissingBones(animation, *model);
  // }

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

  bool FindBoneOffsetInScene(const aiScene &scene, const std::string &boneName,
                             aiMatrix4x4 &outOffset) {
    for (unsigned int i = 0; i < scene.mNumMeshes; i++) {
      aiMesh *mesh = scene.mMeshes[i];
      for (unsigned int j = 0; j < mesh->mNumBones; j++) {
        aiBone *bone = mesh->mBones[j];
        if (bone->mName.C_Str() == boneName) {
          outOffset = bone->mOffsetMatrix;
          return true;
        }
      }
    }
    return false;
  }

  Animation(const aiScene &scene, aiAnimation *animation, std::string name,
            Model *model) {
    this->animation = animation;
    this->name = name;
    m_Duration = animation->mDuration;
    m_TicksPerSecond = animation->mTicksPerSecond;

    meshToChannel.resize(scene.mNumMeshes);
    for (unsigned int meshIndex = 0; meshIndex < scene.mNumMeshes;
         meshIndex++) {
      aiNode *node = FindNodeForMesh(scene.mRootNode, meshIndex);
      if (!node)
        continue;

      aiNodeAnim *channel = nullptr;
      for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        if (animation->mChannels[i]->mNodeName == node->mName) {
          channel = animation->mChannels[i];
          break;
        }
      }

      glm::mat4 transform =
          AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);

      meshToChannel[meshIndex] = {node, channel, transform};
    }

    ReadHierarchyData(m_RootNode, scene.mRootNode);
    ReadMissingBones(animation, *model);

    for (unsigned int i = 0; i < animation->mNumChannels; i++) {
      aiNodeAnim *channel = animation->mChannels[i];
      std::string boneName = channel->mNodeName.data;

      // Make sure the bone exists in the model’s bone info map
      if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
        m_BoneInfoMap[boneName].id = m_BoneInfoMap.size();
        aiMatrix4x4 offsetMatrix; // Assimp matrix

        // You must call a function that finds the aiBone* for this boneName
        // across all meshes in the scene and returns its mOffsetMatrix.
        bool found = FindBoneOffsetInScene(scene, boneName, offsetMatrix);

        if (found) {
          m_BoneInfoMap[boneName].offset =
              AssimpGLMHelpers::ConvertMatrixToGLMFormat(offsetMatrix);
        } else {
          std::cerr << "Warning: Could not find offset for bone: " << boneName
                    << ". Using identity matrix. Rigging issues likely."
                    << std::endl;
          m_BoneInfoMap[boneName].offset = glm::mat4(1.0f);
        }
      }

      // Add a Bone object for this channel
      Bone newBone(boneName, m_BoneInfoMap[boneName].id, channel);
      std::cout << "Found boneName: " << boneName << std::endl;
      m_Bones.push_back(newBone);
    }

    aiMatrix4x4 globalTransformation = scene.mRootNode->mTransformation;
    globalTransformation = globalTransformation.Inverse();
    m_GlobalInverseTransform =
        AssimpGLMHelpers::ConvertMatrixToGLMFormat(globalTransformation);
  }

  Bone *FindBone(const std::string &name) {
    auto iter =
        std::find_if(m_Bones.begin(), m_Bones.end(), [&](const Bone &Bone) {
          return Bone.GetBoneName() == name;
        });
    if (iter == m_Bones.end())
      return nullptr;
    else
      return &(*iter);
  }

  inline float GetTicksPerSecond() { return m_TicksPerSecond; }
  inline float GetDuration() { return m_Duration; }
  inline const AssimpNodeData &GetRootNode() { return m_RootNode; }
  inline const std::map<std::string, BoneInfo> &GetBoneIDMap() {
    return m_BoneInfoMap;
  }

private:
  void ReadMissingBones(const aiAnimation *animation, Model &model) {
    int size = animation->mNumChannels;

    auto &boneInfoMap = model.GetBoneInfoMap(); // getting m_BoneInfoMap from
                                                // ModelAnimation class
    int &boneCount = model.GetBoneCount(); // getting the m_BoneCounter from
                                           // ModelAnimation class

    // reading channels(bones engaged in an animation and their keyframes)
    for (int i = 0; i < size; i++) {
      auto channel = animation->mChannels[i];
      std::string boneName = channel->mNodeName.data;

      if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
        boneInfoMap[boneName].id = boneCount;
        boneCount++;
      }
      m_Bones.push_back(Bone(channel->mNodeName.data,
                             boneInfoMap[channel->mNodeName.data].id, channel));
    }

    m_BoneInfoMap = boneInfoMap;
  }

  void ReadHierarchyData(AssimpNodeData &dest, const aiNode *src) {
    assert(src);

    dest.name = src->mName.data;
    dest.localTransformation =
        AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (unsigned int i = 0; i < src->mNumChildren; i++) {
      AssimpNodeData newData;
      ReadHierarchyData(newData, src->mChildren[i]);
      dest.children.push_back(newData);
    }
  }
  std::vector<Bone> m_Bones;
  AssimpNodeData m_RootNode;
  std::map<std::string, BoneInfo> m_BoneInfoMap;
};
