#pragma once

#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

  static glm::vec3 LerpPosition(aiNodeAnim *channel, float animationTime) {
    if (channel->mNumPositionKeys == 1)
      return GetGLMVec(channel->mPositionKeys[0].mValue);

    // Find the two keyframes surrounding animationTime
    unsigned int index = 0;
    for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; i++) {
      if (animationTime < (float)channel->mPositionKeys[i + 1].mTime) {
        index = i;
        break;
      }
    }

    unsigned int nextIndex = index + 1;
    float deltaTime = (float)(channel->mPositionKeys[nextIndex].mTime -
                              channel->mPositionKeys[index].mTime);
    float factor =
        (animationTime - (float)channel->mPositionKeys[index].mTime) /
        deltaTime;

    glm::vec3 start = GetGLMVec(channel->mPositionKeys[index].mValue);
    glm::vec3 end = GetGLMVec(channel->mPositionKeys[nextIndex].mValue);

    return glm::mix(start, end, factor);
  }

  static glm::quat SlerpRotation(aiNodeAnim *channel, float animationTime) {
    // no rotation keys -> identity
    if (channel->mNumRotationKeys == 0)
      return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // only one key -> return it
    if (channel->mNumRotationKeys == 1)
      return glm::normalize(GetGLMQuat(channel->mRotationKeys[0].mValue));

    // if animationTime is before first key, return first key
    double firstTime = channel->mRotationKeys[0].mTime;
    double lastTime =
        channel->mRotationKeys[channel->mNumRotationKeys - 1].mTime;
    if (animationTime <= (float)firstTime)
      return glm::normalize(GetGLMQuat(channel->mRotationKeys[0].mValue));
    if (animationTime >= (float)lastTime)
      return glm::normalize(GetGLMQuat(
          channel->mRotationKeys[channel->mNumRotationKeys - 1].mValue));

    // find the interval [index, nextIndex] such that key[index].time <=
    // animationTime < key[nextIndex].time
    unsigned int index = 0;
    for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; ++i) {
      double t0 = channel->mRotationKeys[i].mTime;
      double t1 = channel->mRotationKeys[i + 1].mTime;
      if (animationTime >= (float)t0 && animationTime < (float)t1) {
        index = i;
        break;
      }
    }

    unsigned int nextIndex = index + 1;
    float t0 = (float)channel->mRotationKeys[index].mTime;
    float t1 = (float)channel->mRotationKeys[nextIndex].mTime;
    float delta = t1 - t0;

    // safety: if delta is zero, return the start key
    if (delta <= 0.0f)
      return glm::normalize(GetGLMQuat(channel->mRotationKeys[index].mValue));

    float factor = (animationTime - t0) / delta;
    factor = glm::clamp(factor, 0.0f, 1.0f);

    glm::quat qStart =
        glm::normalize(GetGLMQuat(channel->mRotationKeys[index].mValue));
    glm::quat qEnd =
        glm::normalize(GetGLMQuat(channel->mRotationKeys[nextIndex].mValue));

    // ensure shortest path: if dot < 0, negate end quaternion
    if (glm::dot(qStart, qEnd) < 0.0f)
      qEnd = -qEnd;

    glm::quat result = glm::slerp(qStart, qEnd, factor);
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