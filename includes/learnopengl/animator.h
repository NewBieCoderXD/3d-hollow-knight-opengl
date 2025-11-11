#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cstddef>
#include <glm/glm.hpp>
#include <learnopengl/animation.h>
#include <learnopengl/bone.h>
#include <map>
#include <optional>
#include <vector>

enum class AnimationRunType { FORWARD, BACKWARD, FORWARD_AND_BACKWARD };

class Animator : public IAnimator {
public:
  Animation *m_CurrentAnimation;
  float m_CurrentTime;
  float duration;
  float m_DeltaTime;
  AnimationRunType type;
  bool clearAfterDone;
  std::unordered_map<std::string, glm::mat4> m_GlobalNodeTransforms;

  // void setAnimation(Animation *animation, bool reverse) {
  //   m_CurrentTime = 0.0;
  //   m_CurrentAnimation = animation;
  //   this->reverse = reverse;
  //   duration = animation->m_Duration;
  //   if (reverse) {
  //     duration = 2 * animation->m_Duration;
  //   }
  // }

  // float changeSign() { return (m_CurrentTime / duration - 0.5 >= 0) ? 1 : -1;
  // }

  bool isOver() { return this->m_CurrentTime >= duration; }

  float getFrame() {
    if (type == AnimationRunType::FORWARD_AND_BACKWARD) {
      return duration / 2.0f - std::abs(m_CurrentTime - duration / 2.0f);
    }
    float frame = m_CurrentTime;

    if (!clearAfterDone && m_CurrentTime > duration) {
      frame = duration - 0.1f;
    }
    if (type == AnimationRunType::BACKWARD) {
      frame = duration - frame;
    }
    return frame;
  }

  Animator(Animation *animation) {
    m_CurrentTime = 0.0;
    m_CurrentAnimation = animation;

    m_FinalBoneMatrices.reserve(100);

    for (int i = 0; i < 100; i++)
      m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
  }

  // Use updateTimeAndAnim instead
  // void UpdateAnimation(float dt) {
  //   m_DeltaTime = dt;
  //   if (m_CurrentAnimation) {
  //     m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
  //     m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
  //     CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(),
  //                            glm::mat4(1.0f));
  //   }
  // }

  void
  PlayAnimation(Animation *pAnimation, AnimationRunType type,
                std::unordered_map<std::string, glm::mat4> meshNodeTransforms,
                bool clearAfterDone) {
    this->m_CurrentAnimation = pAnimation;
    this->m_CurrentTime = 0.0f;
    this->duration = pAnimation->m_Duration;
    this->type = type;
    this->clearAfterDone = clearAfterDone;
    this->m_GlobalNodeTransforms = meshNodeTransforms;
    // m_BoneInfo.clear();
    // for (unsigned int i = 0; i < pAnimation->meshToChannel.size(); ++i) {
    //   const MeshAnimationChannel &meshChannel = pAnimation->meshToChannel[i];

    //   if (meshChannel.channel) {
    //     // Use the node name of the channel as key
    //     std::string boneName = meshChannel.node->mName.C_Str();
    //     m_BoneInfo[boneName] = meshChannel;
    //   }
    // }
    if (type == AnimationRunType::FORWARD_AND_BACKWARD) {
      duration = 2.0f * pAnimation->m_Duration;
    }
  }

  void updateAnim(float deltaTime) {
    if (m_CurrentAnimation != nullptr) {
      float ticksPerSecond = m_CurrentAnimation->m_TicksPerSecond != 0
                                 ? m_CurrentAnimation->m_TicksPerSecond
                                 : 25.0f;
      // float ticksPerSecond = 50.0f;

      // Accumulate animation time internally
      this->m_CurrentTime += deltaTime * ticksPerSecond;

      // Wrap around the animation duration
      if (clearAfterDone && this->m_CurrentTime > this->duration) {
        this->m_CurrentAnimation = nullptr;
        std::cout << "time over" << std::endl;
        CalculateBoneTransform(nullptr, glm::mat4(1.0f));
      } else {
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(),
                               glm::mat4(1.0f));
      }
    }
  }

  std::optional<glm::mat4> getMeshTransform(unsigned int meshIndex,
                                            float timeInTicks) {
    if (!m_CurrentAnimation) {
      return std::nullopt;
    }

    aiNodeAnim *channel = m_CurrentAnimation->meshToChannel[meshIndex].channel;
    if (!channel)
      return std::nullopt;

    glm::vec3 pos = AssimpGLMHelpers::LerpPosition(channel, timeInTicks);
    glm::quat rot = AssimpGLMHelpers::SlerpRotation(channel, timeInTicks);
    glm::vec3 scale = AssimpGLMHelpers::LerpScale(channel, timeInTicks);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos);
    transform *= glm::toMat4(rot);
    transform = glm::scale(transform, scale);

    return transform;
  }

  // std::optional<glm::mat4> getBoneTransform(const std::string &boneName,
  //                                           float timeInTicks) {
  //   // Find the animation channel for this bone
  //   auto it = m_BoneInfo.find(boneName);
  //   if (it == m_BoneInfo.end()) {
  //     return std::nullopt; // No animation for this bone
  //   }

  //   aiNodeAnim *channel = it->second.channel;
  //   if (!channel)
  //     return std::nullopt;

  //   // Interpolate position
  //   glm::vec3 pos = AssimpGLMHelpers::LerpPosition(channel, timeInTicks);

  //   // Interpolate rotation
  //   glm::quat rot = AssimpGLMHelpers::SlerpRotation(channel, timeInTicks);

  //   // Interpolate scale
  //   glm::vec3 scale = AssimpGLMHelpers::LerpScale(channel, timeInTicks);

  //   // Build transform matrix
  //   glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos);
  //   transform *= glm::toMat4(rot);
  //   transform = glm::scale(transform, scale);

  //   return transform;
  // }

  // TODO! Apply Model Transformation
  void CalculateBoneTransform(const AssimpNodeData *node,
                              glm::mat4 parentTransform) {
    if (node == nullptr) {
      return;
    }
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->localTransformation;

    Bone *Bone = m_CurrentAnimation->FindBone(nodeName);

    if (Bone) {
      Bone->Update(getFrame());
      nodeTransform = Bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    m_GlobalNodeTransforms[nodeName] = globalTransformation;

    auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    if (boneInfoMap.count(nodeName)) {
      int boneID = boneInfoMap.at(nodeName).id;
      glm::mat4 offsetMatrix = boneInfoMap.at(nodeName).offset;
      // The final skinning matrix (to deform vertices in the shader)
      m_FinalBoneMatrices[boneID] =
          m_CurrentAnimation->m_GlobalInverseTransform * globalTransformation *
          offsetMatrix;
    }

    for (int i = 0; i < node->childrenCount; i++)
      CalculateBoneTransform(&node->children[i], globalTransformation);
  }

  std::optional<glm::mat4> GetGlobalNodeTransform(std::string nodeName) {
    auto found = m_GlobalNodeTransforms.find(nodeName);
    if (found != m_GlobalNodeTransforms.end()) {
      return found->second;
    }
    return {};
  }

  std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

  Animation *GetAnimation() { return m_CurrentAnimation; }

private:
  std::vector<glm::mat4> m_FinalBoneMatrices;
};
