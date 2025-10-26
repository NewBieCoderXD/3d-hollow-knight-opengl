#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <learnopengl/animation.h>
#include <learnopengl/bone.h>
#include <map>
#include <vector>

class Animator {
public:
  Animation *m_CurrentAnimation;
  float m_CurrentTime;
  float duration;
  float m_DeltaTime;
  bool reverse;
  bool isRevesing;

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

  float getFrame() {
    if (reverse) {
      return duration / 2.0f - std::abs(m_CurrentTime - duration / 2.0f);
    }
    return m_CurrentTime;
  }

  Animator(Animation *animation) {
    m_CurrentTime = 0.0;
    m_CurrentAnimation = animation;

    m_FinalBoneMatrices.reserve(100);

    for (int i = 0; i < 100; i++)
      m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
  }

  void UpdateAnimation(float dt) {
    m_DeltaTime = dt;
    if (m_CurrentAnimation) {
      m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
      m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
      CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(),
                             glm::mat4(1.0f));
    }
  }

  void PlayAnimation(Animation *pAnimation, bool reverse) {
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
    duration = pAnimation->m_Duration;
    this->reverse = reverse;
    if (reverse) {
      duration = 2.0f * pAnimation->m_Duration;
    }
  }

  void CalculateBoneTransform(const AssimpNodeData *node,
                              glm::mat4 parentTransform) {
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone *Bone = m_CurrentAnimation->FindBone(nodeName);

    if (Bone) {
      Bone->Update(m_CurrentTime);
      nodeTransform = Bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
      int index = boneInfoMap[nodeName].id;
      glm::mat4 offset = boneInfoMap[nodeName].offset;
      m_FinalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; i++)
      CalculateBoneTransform(&node->children[i], globalTransformation);
  }

  std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

  Animation *GetAnimation() { return m_CurrentAnimation; }

private:
  std::vector<glm::mat4> m_FinalBoneMatrices;
};
