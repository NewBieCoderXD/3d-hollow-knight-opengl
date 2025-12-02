
#include "../constants.cpp"
#include "assimp/anim.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "learnopengl/animation.h"
#include "learnopengl/animator.h"
#include "learnopengl/assimp_glm_helpers.h"
#include "learnopengl/bone.h"
#include "learnopengl/box.hpp"
#include "learnopengl/model_animation.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <learnopengl/filesystem.h>
#include <memory>
#include <optional>

class ModelAnimationAbs {
public:
  std::unique_ptr<Model> model;
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 scale = glm::vec3(1.0f);

  glm::vec3 velocity{0.0f, 0.0f, 0.0f};
  glm::vec3 acceleration{0.0f, 0.0f, 0.0f};
  float friction = 8.0f;

  glm::vec3 modelSize = glm::vec3(0.0f);
  float health = 5.0f;
  float maxHealth = 5.0f;

  std::string weaponNodeName = "";
  // glm::vec3 weaponSize = glm::vec3(0.0f);
  // std::unique_ptr<DebugBox> weaponHitbox;

  float lastHit = 0.0f;
  std::unique_ptr<DebugBox> hitbox;
  const bool showHitbox = true;

  Animator animator;
  const aiScene *scene;

  ModelAnimationAbs(Assimp::Importer &importer, const std::string &path,
                    std::string name, std::string weaponMesh,
                    glm::vec3 position = glm::vec3(0.0f),
                    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                    glm::vec3 scale = glm::vec3(1.0f), bool gamma = false)
      : position(position), rotation(rotation), scale(scale), animator(NULL) {
    this->weaponNodeName = weaponMesh;

    if (!showHitbox) {
      weaponMesh = "";
    }

    scene = importer.ReadFile(
        FileSystem::getPath(path),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes);
    std::cout << "Loading model: " << path << std::endl;
    this->model = std::make_unique<Model>(
        Model(scene, path.substr(0, path.find_last_of('/')), scale, name,
              weaponMesh, false));
    this->animator.m_GlobalNodeTransforms = this->model->meshNodeTransforms;

    aiVector3D rootMin(FLT_MAX, FLT_MAX, FLT_MAX);
    aiVector3D rootMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    ComputeBoundingBox(scene, scene->mRootNode, rootMin, rootMax,
                       aiMatrix4x4());
    glm::vec3 rootMinGlm = AssimpGLMHelpers::GetGLMVec(rootMin);
    glm::vec3 rootMaxGlm = AssimpGLMHelpers::GetGLMVec(rootMax);
    modelSize = (rootMaxGlm - rootMinGlm) * scale;
    glm::vec3 rootHalfSize = modelSize * 0.5f;

    glm::vec3 weaponHalfSize = glm::vec3(0.0f);
    glm::vec3 weaponSize = glm::vec3(0.0f);
    aiNode *weaponNode = FindAiNodeByName(scene->mRootNode, weaponMesh);
    if (weaponNode) {
      aiVector3D weaponMin(FLT_MAX, FLT_MAX, FLT_MAX);
      aiVector3D weaponMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
      ComputeBoundingBox(scene, FindAiNodeByName(scene->mRootNode, weaponMesh),
                         rootMin, rootMax, aiMatrix4x4());
      glm::vec3 weaponMinGlm = AssimpGLMHelpers::GetGLMVec(rootMin);
      glm::vec3 weaponMaxGlm = AssimpGLMHelpers::GetGLMVec(rootMax);
      weaponSize = (weaponMaxGlm - weaponMinGlm) * scale;
      weaponHalfSize = modelSize * 0.5f;
    }

    if (showHitbox) {
      hitbox = std::make_unique<DebugBox>(-rootHalfSize, rootHalfSize);

      this->model->weaponHitbox =
          std::make_unique<DebugBox>(-weaponHalfSize, weaponHalfSize);
      this->model->weaponSize = weaponSize;
    }

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      aiAnimation *anim = scene->mAnimations[i];
      const char *name = anim->mName.C_Str();
      std::cout << "Found action " << name << std::endl;
      this->nameToAnimation.insert(
          {string(name), Animation(*scene, anim, name, model.get())});
    }
  }

  std::map<std::string, aiNodeAnim *> BuildMeshToChannel(aiAnimation *anim) {
    std::map<std::string, aiNodeAnim *> meshToChannel;

    for (unsigned int i = 0; i < anim->mNumChannels; i++) {
      aiNodeAnim *channel = anim->mChannels[i];
      std::string nodeName = channel->mNodeName.C_Str();

      meshToChannel[nodeName] = {channel};
    }

    return meshToChannel;
  }

  aiNode *FindAiNodeByName(aiNode *node, const std::string &name) {
    if (!node)
      return nullptr;

    if (node->mName.C_Str() == name) // Compare with aiString
      return node;

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      aiNode *found = FindAiNodeByName(node->mChildren[i], name);
      if (found)
        return found;
    }

    return nullptr;
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

  void setAnimation(std::string name, AnimationRunType type,
                    bool clearAfterDone) {
    auto animationItr = this->nameToAnimation.find(name);
    if (animationItr != this->nameToAnimation.end()) {
      std::cout << "playing animation " << name << std::endl;
      this->animator.PlayAnimation(&animationItr->second, type,
                                   this->model->meshNodeTransforms,
                                   clearAfterDone);
    } else {
      std::cout << "animation not found" << std::endl;
    }
  }

  void updatePosition(float deltaTime) {
    if (glm::length(this->velocity) > 0.01) {
      this->position += this->velocity * deltaTime;
      this->position.y = std::max(this->position.y, 0.0f);
    }
    this->velocity += this->acceleration * deltaTime;
    this->velocity *= (1 - this->friction * deltaTime);
    // if (this->model->name == "hornet") {
    //   std::cout << glm::to_string(this->velocity) << std::endl;
    // }
  }

  void draw(glm::mat4 parentMtx, glm::mat4 projection, glm::mat4 view,
            Shader &shader, Shader &hitboxShader, float deltaTime,
            float lastFrame) {
    if (health <= 0) {
      return;
    }
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    GLint isHitLocation = glGetUniformLocation(shader.ID, "isHit");

    if (lastFrame < DAMAGE_EFFECT + lastHit) {
      // std::cout << "lastFrame: " << lastFrame << " lastHit: " << lastHit
      //           << std::endl;
      glUniform1i(isHitLocation, true);
    } else {
      glUniform1i(isHitLocation, false);
    }

    glm::mat4 modelMtx = glm::translate(parentMtx, position);
    modelMtx *= glm::toMat4(rotation);
    modelMtx = glm::scale(modelMtx, glm::vec3(scale));

    animator.updateAnim(deltaTime);
    std::vector<glm::mat4> finalTransforms = animator.GetFinalBoneMatrices();
    for (size_t i = 0; i < finalTransforms.size(); ++i) {
      glUniformMatrix4fv(
          glGetUniformLocation(
              shader.ID,
              ("finalBonesMatrices[" + std::to_string(i) + "]").c_str()),
          1, GL_FALSE, glm::value_ptr(finalTransforms[i]));
    }

    // if (weaponMesh == "hornet.008") {
    //   std::cout << "anim" << animator.GetAnimation() << std::endl;
    // }
    model->Draw(modelMtx, projection, view, animator, shader, hitboxShader,
                showHitbox);
    if (showHitbox) {
      hitboxShader.use();
      hitboxShader.setMat4("projection", projection);
      hitboxShader.setMat4("view", view);

      glm::vec3 pos = glm::vec3(modelMtx[3]);
      pos.y += modelSize.y / 2.0;
      // hitbox->setVisible(true);
      // hitbox->draw(glm::translate(glm::mat4(1.0f), pos), hitboxShader);

      if (showHitbox && this->model->weaponHitbox != nullptr &&
          this->weaponNodeName != "") {
        // hitboxShader.use();
        // hitboxShader.setMat4("projection", projection);
        // hitboxShader.setMat4("view", view);

        auto boneTransform =
            animator.GetGlobalNodeTransform(this->weaponNodeName);
        glm::mat4 localTransform = boneTransform.value();
        // if (boneTransform.has_value()) {
        //   localTransform = boneTransform.value();
        // } else {
        //   localTransform = this->model->meshNodeTransforms[i];
        // }
        glm::mat4 weaponGlobal = modelMtx * localTransform;
        this->model->weaponPos = glm::vec3(weaponGlobal[3]);
        // if (this->name == "hornet") {
        //   // std::cout << animator.GetGlobalNodeTransforms().size() <<
        //   // std::endl; for (auto pair : animator.GetGlobalNodeTransforms())
        //   {
        //   //   std::cout << pair << " " << std::endl;
        //   // }
        //   // std::cout << glm::to_string(localTransform)
        //   //           << (boneTransform.has_value() ? "has bones" : "")
        //   //           << std::endl;
        // }
        
        // this->model->weaponHitbox->setVisible(true);
        // this->model->weaponHitbox->draw(
        //     glm::translate(glm::mat4(1.0f), this->model->weaponPos),
        //     hitboxShader);
      }
    }
  }

  glm::vec3 getWeaponPosition() { return model->weaponPos; }

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