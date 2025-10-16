
#include "learnopengl/animation.h"
#include "learnopengl/animator.h"
#include "learnopengl/model_animation.h"
#include <assimp/Importer.hpp>
#include <learnopengl/filesystem.h>
#include <memory>

class ModelAnimationAbs {
public:
  std::unique_ptr<Model> model;
  Animator animator;
  const aiScene *scene;

  ModelAnimationAbs(Assimp::Importer &importer, const std::string &path,
                    bool gamma = false)
      : animator(NULL) {
    scene =
        importer.ReadFile(FileSystem::getPath(path),
                          aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_CalcTangentSpace);
    this->model = std::make_unique<Model>(
        Model(scene, path.substr(0, path.find_last_of('/')), false));

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
};