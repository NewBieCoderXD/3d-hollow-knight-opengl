
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
  Animator animator;
  const aiScene *scene;

  ModelAnimationAbs(Assimp::Importer &importer, const std::string &path,
                    bool gamma = false)
      : animator(NULL) {
    scene =
        importer.ReadFile(FileSystem::getPath(path),
                          aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_CalcTangentSpace);
    std::cout << "Loading model: " << path << std::endl;
    this->model = std::make_unique<Model>(
        Model(scene, path.substr(0, path.find_last_of('/')), false));

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      aiAnimation *anim = scene->mAnimations[i];
      const char *name = anim->mName.C_Str();
      std::cout << "Found action " << name << std::endl;
      this->nameToAnimation.insert(
          {string(name), Animation(*scene, anim, this->model.get())});
    }
  }

  void setAnimation(std::string name) {
    auto animationItr = this->nameToAnimation.find(name);
    if (animationItr != this->nameToAnimation.end()) {
      std::cout << "playing animation " << name << std::endl;
      this->animator.PlayAnimation(&animationItr->second);
    }
  }

  void draw(glm::mat4 modelMtx, Shader &shader, float deltaTime) {
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
      timeInTicks = animator.m_CurrentTime;
      if (animator.m_CurrentTime > foundAnim->m_Duration) {
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