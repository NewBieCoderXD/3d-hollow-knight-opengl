
#include "learnopengl/animation.h"
#include "learnopengl/animator.h"
#include "learnopengl/model_animation.h"
#include <learnopengl/filesystem.h>
#include <memory>

class ModelAnimationAbs {
public:
  std::unique_ptr<Model> model;

  ModelAnimationAbs(const std::string &path, bool gamma = false)
      : animator(NULL) {

    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(FileSystem::getPath(path),
                          aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_CalcTangentSpace);
    this->model = std::make_unique<Model>(
        Model(scene, path.substr(0, path.find_last_of('/')), false));

    // this->animations=scene->mAnimations;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      aiAnimation *anim = scene->mAnimations[i];
      const char *name = anim->mName.C_Str();
      this->nameToAnimation.insert(
          {string(name), Animation(*scene, anim, this->model.get())});
    }
  }

  void setAnimation(std::string name) {
    auto animationItr = this->nameToAnimation.find(name);
    if (animationItr != this->nameToAnimation.end()) {
      this->animator = Animator(&animationItr->second);
    }
  }

private:
  std::map<string, Animation> nameToAnimation{};
  Animator animator;
};