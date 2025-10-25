#pragma once

#include "glm/fwd.hpp"
#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

class AssimpGLMHelpers {
public:
  static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4 &from) {
    glm::mat4 to;
    // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
  }

  static inline glm::vec3 GetGLMVec(const aiVector3D &vec) {
    return glm::vec3(vec.x, vec.y, vec.z);
  }

  static inline glm::quat GetGLMQuat(const aiQuaternion &pOrientation) {
    return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y,
                     pOrientation.z);
  }

  static glm::vec3 LerpPosition(aiNodeAnim *channel, float timeInTicks) {
    if (channel->mNumPositionKeys == 1)
      return GetGLMVec(channel->mPositionKeys[0].mValue);

    // Find the two keyframes surrounding animationTime
    unsigned int index = 0;
    for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; i++) {
      if (timeInTicks < (float)channel->mPositionKeys[i + 1].mTime) {
        index = i;
        break;
      }
    }

    glm::vec3 start = GetGLMVec(channel->mPositionKeys[index].mValue);

    return start;
    // unsigned int nextIndex = index + 1;
    // float deltaTime = (float)(channel->mPositionKeys[nextIndex].mTime -
    //                           channel->mPositionKeys[index].mTime);
    // float factor =
    //     (animationTime - (float)channel->mPositionKeys[index].mTime) /
    //     deltaTime;

    // glm::vec3 start = GetGLMVec(channel->mPositionKeys[index].mValue);
    // glm::vec3 end = GetGLMVec(channel->mPositionKeys[nextIndex].mValue);

    // return glm::mix(start, end, factor);
  }

  static glm::quat SlerpRotation(aiNodeAnim *channel, float animationTime) {
    if (channel->mNumRotationKeys == 1) {
      // Only one keyframe — just return it
      const aiQuaternion &q = channel->mRotationKeys[0].mValue;
      return glm::quat(q.w, q.x, q.y, q.z);
    }

    // Find the two keyframes between which we will interpolate
    unsigned int rotationIndex = 0;
    for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; i++) {
      if (animationTime <
          static_cast<float>(channel->mRotationKeys[i + 1].mTime)) {
        rotationIndex = i;
        break;
      }
    }

    // aiQuaternion rot = channel->mRotationKeys[rotationIndex].mValue;

    // return glm::quat(rot.w, rot.x, rot.y, rot.z);

    unsigned int nextRotationIndex = rotationIndex + 1;
    float deltaTime =
        static_cast<float>(channel->mRotationKeys[nextRotationIndex].mTime -
                           channel->mRotationKeys[rotationIndex].mTime);

    // Avoid divide by zero if deltaTime is extremely small
    float factor = (deltaTime > 0.0f)
                       ? (animationTime -
                          static_cast<float>(
                              channel->mRotationKeys[rotationIndex].mTime)) /
                             deltaTime
                       : 0.0f;

    // Clamp interpolation factor to [0, 1]
    factor = glm::clamp(factor, 0.0f, 1.0f);

    // Get start and end rotations
    const aiQuaternion &startQ = channel->mRotationKeys[rotationIndex].mValue;
    const aiQuaternion &endQ = channel->mRotationKeys[nextRotationIndex].mValue;

    // Convert to glm::quat
    glm::quat start = glm::quat(startQ.w, startQ.x, startQ.y, startQ.z);
    glm::quat end = glm::quat(endQ.w, endQ.x, endQ.y, endQ.z);

    // Spherical linear interpolation (slerp)
    glm::quat result = glm::slerp(start, end, factor);

    return glm::normalize(result);
  }

  static glm::vec3 LerpScale(aiNodeAnim *channel, float animationTime) {
    if (channel->mNumScalingKeys == 1)
      return GetGLMVec(channel->mScalingKeys[0].mValue);

    unsigned int index = 0;
    for (unsigned int i = 0; i < channel->mNumScalingKeys - 1; i++) {
      if (animationTime < (float)channel->mScalingKeys[i + 1].mTime) {
        index = i;
        break;
      }
    }

    unsigned int nextIndex = index + 1;
    float deltaTime = (float)(channel->mScalingKeys[nextIndex].mTime -
                              channel->mScalingKeys[index].mTime);
    float factor =
        (animationTime - (float)channel->mScalingKeys[index].mTime) / deltaTime;

    glm::vec3 start = GetGLMVec(channel->mScalingKeys[index].mValue);
    glm::vec3 end = GetGLMVec(channel->mScalingKeys[nextIndex].mValue);

    return glm::mix(start, end, factor);
  }
};