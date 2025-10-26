
#include "../constants.cpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "learnopengl/animation.h"
#include "learnopengl/animator.h"
#include "learnopengl/bone.h"
#include "learnopengl/model_animation.h"
#include <assimp/Importer.hpp>
#include <learnopengl/filesystem.h>
#include <memory>

class ModelAnimationAbs {
public:
  std::unique_ptr<Model> model;
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
  float width;
  float height;
  float lastHit = 0.0f;

  Animator animator;
  const aiScene *scene;

  ModelAnimationAbs(Assimp::Importer &importer, const std::string &path,
                    bool gamma = false)
      : animator(NULL) {
    scene = importer.ReadFile(
        FileSystem::getPath(path),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes);
    std::cout << "Loading model: " << path << std::endl;
    this->model = std::make_unique<Model>(
        Model(scene, path.substr(0, path.find_last_of('/')), false));

    aiVector3D min(FLT_MAX, FLT_MAX, FLT_MAX);
    aiVector3D max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    ComputeBoundingBox(scene, scene->mRootNode, min, max, aiMatrix4x4());
    width = max.x - min.x;
    height = max.z - min.z;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      aiAnimation *anim = scene->mAnimations[i];
      const char *name = anim->mName.C_Str();
      std::cout << "Found action " << name << std::endl;
      this->nameToAnimation.insert(
          {string(name), Animation(*scene, anim, this->model.get())});
    }
  }

  void ComputeBoundingBox(const aiScene *scene, const aiNode *node,
                          aiVector3D &min, aiVector3D &max,
                          const aiMatrix4x4 &parentTransform) {
    aiMatrix4x4 currentTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

      aiVector3D aabbMin = mesh->mAABB.mMin;
      aiVector3D aabbMax = mesh->mAABB.mMax;

      // Transform the 8 corners of the AABB to world space
      for (int j = 0; j < 8; ++j) {
        aiVector3D corner((j & 1) ? aabbMax.x : aabbMin.x,
                          (j & 2) ? aabbMax.y : aabbMin.y,
                          (j & 4) ? aabbMax.z : aabbMin.z);
        aiVector3D transformed = corner;
        transformed *= currentTransform;
        min.x = std::min(min.x, transformed.x);
        min.y = std::min(min.y, transformed.y);
        min.z = std::min(min.z, transformed.z);
        max.x = std::max(max.x, transformed.x);
        max.y = std::max(max.y, transformed.y);
        max.z = std::max(max.z, transformed.z);
      }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
      ComputeBoundingBox(scene, node->mChildren[i], min, max, currentTransform);
  }

  void setAnimation(std::string name, bool reverse) {
    auto animationItr = this->nameToAnimation.find(name);
    if (animationItr != this->nameToAnimation.end()) {
      std::cout << "playing animation " << name << std::endl;
      this->animator.PlayAnimation(&animationItr->second, reverse);
    } else {
      std::cout << "animation not found" << std::endl;
    }
  }

  void draw(glm::mat4 modelMtx, Shader &shader, float deltaTime,
            float lastFrame) {
    GLint isHitLocation = glGetUniformLocation(shader.ID, "isHit");

    shader.use();
    if (lastFrame < DAMAGE_EFFECT + lastHit) {
      // std::cout << "lastFrame: " << lastFrame << " lastHit: " << lastHit
      //           << std::endl;
      glUniform1i(isHitLocation, true);
    } else {
      glUniform1i(isHitLocation, false);
    }

    modelMtx = glm::translate(modelMtx, position);
    modelMtx *= glm::toMat4(rotation);
    modelMtx = glm::scale(modelMtx, glm::vec3(scale));

    float timeInTicks = deltaTime;

    Animation *foundAnim = animator.GetAnimation();
    if (foundAnim != nullptr) {
      float ticksPerSecond = foundAnim->m_TicksPerSecond != 0
                                 ? foundAnim->m_TicksPerSecond
                                 : 25.0f;
      // float ticksPerSecond = 50.0f;

      // Accumulate animation time internally
      animator.m_CurrentTime += deltaTime * ticksPerSecond;

      // Wrap around the animation duration
      timeInTicks = animator.getFrame();
      if (animator.m_CurrentTime > animator.duration) {
        animator.m_CurrentAnimation = nullptr;
      }
    }
    model->Draw(modelMtx, shader, timeInTicks);
  }

  glm::vec3 getFront() {
    // The local forward vector (model-space)
    glm::vec3 localForward = glm::vec3(0.0f, 0.0f, 1.0f);

    // Rotate it by the quaternion to get world-space forward
    glm::vec3 worldForward = rotation * localForward;

    // Ensure it is normalized
    return glm::normalize(worldForward);
  }

private:
  std::map<string, Animation> nameToAnimation{};
};