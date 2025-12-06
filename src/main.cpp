#include "glm/geometric.hpp"
#include "learnopengl/bar.h"
#include "learnopengl/grass_field.hpp"
#include "learnopengl/group_plane.hpp"
#include "learnopengl/model_animation.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/grass_field.hpp>
#include <learnopengl/model.h>
#include <learnopengl/model_animation_abstraction.h>
#include <learnopengl/shader_m.h>

#include <learnopengl/animator.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <random>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "../includes/learnopengl/cubemap.hpp"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, float deltaTime);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

ma_engine audioEngine;
std::unordered_map<std::string, std::unique_ptr<ma_sound>> preLoadedSounds;

float firstRender = 0;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::optional<ModelAnimationAbs> knight;
std::optional<ModelAnimationAbs> hornet;

inline bool CheckAABBCollision(const glm::vec3 &posA, const glm::vec3 &sizeA,
                               const glm::vec3 &posB, const glm::vec3 &sizeB) {
  // Half extents
  glm::vec3 halfA = sizeA * 0.5f;
  glm::vec3 halfB = sizeB * 0.5f;

  bool collisionX = std::abs(posA.x - posB.x) <= (halfA.x + halfB.x);
  bool collisionY = std::abs(posA.y - posB.y) <= (halfA.y + halfB.y);
  bool collisionZ = std::abs(posA.z - posB.z) <= (halfA.z + halfB.z);

  return collisionX && collisionY && collisionZ;
}

// bool on_menu = true;

enum class MenuType { START_MENU, LOSE, WIN, PLAYING };

MenuType menu_state = MenuType::START_MENU;

enum class HornetState {
  LUNGE_WAIT,
  LUNGE,
  DASH_WAIT,
  DASH,
  JUMP_WAIT,
  JUMP,
  JUMP_DOWN,
  IDLE,
  DEAD
};

HornetState hornetState = HornetState::IDLE;
float lastHornetStateSet = 0.0f;
float lastHornetAttack = 0.0f;

const float maxHealth = 5.0;
float currentHealth = maxHealth;

enum class KnightState { ATTACKING, IDLE };
KnightState knightState = KnightState::IDLE;
float lastKnightStateSet = 0.0f;

std::default_random_engine randomEngine;
std::uniform_real_distribution<float> toOneDist(0.0f, 1.0f);
std::uniform_real_distribution<float> toTenDist(0.0f, 10.f);

std::unique_ptr<HealthBar> playerHealth;

GLFWwindow *window;

ImGuiIO *imguiIO;
ImFont *font24;
ImFont *font50;
ImFont *font100;

float game_over_time = 0;

void randomHornetState() {
  if (hornetState == HornetState::DEAD) {
    return;
  }

  // hornetState = HornetState::LUNGE_WAIT;

  if (glm::length(hornet->position - knight->position) < 6.0) {
    float action = toTenDist(randomEngine);
    // std::cout << action << std::endl;
    // int action = (std::rand() % 3);
    // hornetState = static_cast<HornetState>(action);
    if (action < 8.0) {
      hornetState = HornetState::LUNGE_WAIT;
    } else if (action < 9.0) {
      hornetState = HornetState::JUMP_WAIT;
    } else {
      hornetState = HornetState::DASH_WAIT;
    }
  } else {
    float action = toTenDist(randomEngine);
    if (action < 5.0) {
      hornetState = HornetState::JUMP_WAIT;
    } else {
      hornetState = HornetState::DASH_WAIT;
    }
  }
  lastHornetAttack = lastFrame;

  glm::mat4 lookAtMatrix = glm::lookAt(hornet->position, knight->position,
                                       glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat3 rotationMatrix = glm::mat3(glm::transpose(lookAtMatrix));

  switch (hornetState) {
  case HornetState::LUNGE_WAIT: {
    hornet->rotation = glm::quat_cast(rotationMatrix);
    ma_sound_start(preLoadedSounds[AUDIO_SHAW].get());
    lastHornetStateSet = lastFrame;
    break;
  }
  case HornetState::IDLE:
  case HornetState::JUMP:
  case HornetState::JUMP_DOWN:
  case HornetState::DASH:
  case HornetState::DEAD:
  case HornetState::LUNGE: {
    break;
  }
  case HornetState::DASH_WAIT: {
    hornet->rotation = glm::quat_cast(rotationMatrix);
    ma_sound_start(preLoadedSounds[AUDIO_SHAW].get());
    hornet->setAnimation("point_forward", AnimationRunType::FORWARD, false);
    lastHornetStateSet = lastFrame;
    break;
  }
  case HornetState::JUMP_WAIT: {
    glm::vec3 above = hornet->position;
    float radius = 5.0f;
    float random_y = 2.0;
    float angle = toOneDist(randomEngine) * glm::two_pi<float>();
    float random_x = cos(angle) * radius;
    float random_z = sin(angle) * radius;
    above = above + glm::vec3(random_x, random_y, random_z);

    glm::vec3 knightPos = knight->position;
    float minDistance = 5.0f;

    glm::vec3 dir = above - knightPos;
    float dist = glm::length(dir);

    // If too close, push it further away
    if (dist < minDistance) {
      dir = glm::normalize(dir);
      above = knightPos + dir * minDistance;
    }

    // create rotation matrix from direction
    glm::mat4 lookAtMatrix =
        glm::lookAt(hornet->position, above, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat3 rotationMatrix = glm::mat3(glm::transpose(lookAtMatrix));
    hornet->rotation = glm::quat_cast(rotationMatrix);

    ma_sound_start(preLoadedSounds[AUDIO_EDINO].get());
    hornet->setAnimation("point_forward", AnimationRunType::FORWARD, false);
    lastHornetStateSet = lastFrame;
    break;
  }
  }
  lastHornetAttack = lastFrame;
}

void updateHornetState() {
  switch (hornetState) {
  case HornetState::LUNGE_WAIT: {
    if (lastFrame > lastHornetStateSet + 0.5f) {
      hornetState = HornetState::LUNGE;
      hornet->setAnimation("lunge", AnimationRunType::FORWARD_AND_BACKWARD,
                           true);
      lastHornetStateSet = lastFrame;
    }
    break;
  }
  case HornetState::LUNGE: {
    if (hornet->animator.isOver() || lastFrame > lastHornetStateSet + 3.0f) {
      hornet->setAnimation("point_forward", AnimationRunType::BACKWARD, true);
      hornetState = HornetState::IDLE;
    }
    break;
  }
  case HornetState::DASH_WAIT: {
    if (lastFrame > lastHornetStateSet + 0.5f) {
      hornetState = HornetState::DASH;
      lastHornetStateSet = lastFrame;
    }
    break;
  }
  case HornetState::DASH: {
    hornet->velocity -= hornet->getFront() * HORNET_DASH_SPEED;
    if (lastFrame > lastHornetStateSet + 0.3f) {
      hornetState = HornetState::IDLE;
      lastHornetStateSet = lastFrame;
    }
    break;
  }
  case HornetState::JUMP_WAIT: {
    if (lastFrame > lastHornetStateSet + 0.5f) {
      hornetState = HornetState::JUMP;
      lastHornetStateSet = lastFrame;
    }
    break;
  }
  case HornetState::JUMP: {
    if (lastFrame < lastHornetStateSet + 1.2f && hornet->position.y < 4.0f) {
      hornet->velocity -= hornet->getFront() * HORNET_DASH_SPEED;
    } else {
      glm::mat4 lookAtMatrix = glm::lookAt(hornet->position, knight->position,
                                           glm::vec3(0.0f, 1.0f, 0.0f));
      glm::mat3 rotationMatrix = glm::mat3(glm::transpose(lookAtMatrix));
      hornet->rotation = glm::quat_cast(rotationMatrix);
    }
    if (lastFrame > lastHornetStateSet + 1.0f) {
      ma_sound_start(preLoadedSounds[AUDIO_HAA].get());
      hornetState = HornetState::JUMP_DOWN;
      lastHornetStateSet = lastFrame;
    }
    break;
  }
  case HornetState::JUMP_DOWN: {
    if (lastFrame > lastHornetStateSet + 0.5f) {
      if (hornet->position.y > 0.01f) {
        hornet->velocity -= hornet->getFront() * HORNET_DASH_SPEED;
      }
      if (lastFrame > lastHornetStateSet + 3.0f) {
        hornetState = HornetState::IDLE;
        lastHornetStateSet = lastFrame;
      }
    }
    break;
  }
  case HornetState::DEAD:
  case HornetState::IDLE:
    break;
  }
}

void updateKnightState() {
  switch (knightState) {
  case KnightState::ATTACKING: {
    if (lastFrame > KNIGHT_ATTACK_DELAY + lastKnightStateSet) {
      knightState = KnightState::IDLE;
    }
    break;
  }
  default:
    break;
  }
}

void new_sound(std::unique_ptr<ma_sound> &sound, std::string file,
               float volume) {
  ;
  auto ma_result = ma_sound_init_from_file(&audioEngine, file.c_str(), 0, NULL,
                                           NULL, sound.get());
  assert(ma_result == MA_SUCCESS);
  ma_sound_set_volume(sound.get(), volume);
}

void onStartGame() {
  hornetState = HornetState::IDLE;
  hornet->health = 15;
  hornet->position = glm::vec3(0.0, 0.0, 0.0);
  hornet->velocity = glm::vec3(0.0);
  lastHornetStateSet = 0.0f;
  lastHornetAttack = 0.0f;

  knightState = KnightState::IDLE;
  currentHealth = 5;
  lastKnightStateSet = 0.0f;
  knight->position = glm::vec3(3.0, 0.0, 0.0);
  knight->velocity = glm::vec3(0.0);
}

void centerText(const char *text) {
  float text_width = ImGui::CalcTextSize(text).x;
  ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_width) * 0.5f);
  ImGui::Text("%s", text);
}

void RenderMenu() {
  // window
  const ImGuiViewport *viewport = ImGui::GetMainViewport();

  // 2. Set the next window's position and size to cover the viewport
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushFont(font50);
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus;
  ImGui::Begin("Main Window", nullptr, window_flags);

  centerText("Hole Low Knight");
  ImGui::Spacing();
  ImGui::Spacing();
  float button_width = ImGui::GetWindowSize().x * 0.2f;
  float button_height = ImGui::GetWindowSize().x * 0.10f;
  ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - (button_width / 2));

  if (ImGui::Button("Start Game", ImVec2(button_width, button_height))) {
    menu_state = MenuType::PLAYING;
    onStartGame();
    std::cout << "Starting the game!" << std::endl;
  }

  ImGui::Spacing();
  ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - (button_width / 2));

  // 2. Settings Button
  if (ImGui::Button("Settings", ImVec2(button_width, button_height))) {
    std::cout << "Opening Settings panel..." << std::endl;
  }

  ImGui::Spacing();
  ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - (button_width / 2));

  if (ImGui::Button("Quit", ImVec2(button_width, button_height))) {
    glfwSetWindowShouldClose(window, true);
    std::cout << "Quitting application." << std::endl;
  }
  ImGui::PopFont();
  ImGui::End();
}

void RenderLoseWin(bool hasWon) {
  // window
  const ImGuiViewport *viewport = ImGui::GetMainViewport();

  // 2. Set the next window's position and size to cover the viewport
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushFont(font100);
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus;
  ImGui::Begin("Main Window", nullptr, window_flags);

  ImGui::SetCursorPosY(ImGui::GetWindowSize().y * 0.5f);
  if (hasWon) {
    centerText("YOU WON!!");
  } else {
    centerText("Better Luck Next Time~~");
  }

  ImGui::PopFont();
  ImGui::End();
}

void playerTakeDamage(uint damage) {
  currentHealth -= damage;
  if (currentHealth == 0) {
    menu_state = MenuType::LOSE;
    game_over_time = static_cast<float>(glfwGetTime());
  }
  playerHealth->setHealth(currentHealth, maxHealth);
}

int main() {
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  SCR_WIDTH = mode->width;
  SCR_HEIGHT = mode->height;
  window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL",
                            glfwGetPrimaryMonitor(), NULL);
  if (window == NULL) {
    const char *description;
    int code = glfwGetError(&description);
    if (description) {
      // Log or print the error code and description
      printf("GLFW Error %d: %s\n", code, description);
    }
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
  glfwSwapBuffers(window);

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  unsigned seed =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  randomEngine = std::default_random_engine(seed);

  // Audio
  // ----------------------
  ma_result result = ma_engine_init(NULL, &audioEngine);
  assert(result == MA_SUCCESS);

  std::map<std::string, float> soundToVolume = {
      {AUDIO_SHAW, 0.2f}, {AUDIO_EDINO, 0.2f}, {AUDIO_HAA, 0.2f}};
  for (auto [file, volume] : soundToVolume) {
    std::unique_ptr<ma_sound> pSound = std::make_unique<ma_sound>();
    new_sound(pSound, file, volume);
    preLoadedSounds[file] = std::move(pSound);
  }

  // GUI
  // -----------------
  ImGui::CreateContext();
  imguiIO = &ImGui::GetIO();
  font24 = imguiIO->Fonts->AddFontFromFileTTF(
      "resources/fonts/ComicSansMS3.ttf", 24.0f);
  font50 = imguiIO->Fonts->AddFontFromFileTTF(
      "resources/fonts/ComicSansMS3.ttf", 50.0f);
  font100 = imguiIO->Fonts->AddFontFromFileTTF(
      "resources/fonts/ComicSansMS3.ttf", 100.0f);
  ImGui_ImplGlfw_InitForOpenGL(window,
                               true); // 'true' sets up callbacks for input
  ImGui_ImplOpenGL3_Init("#version 330");

  // build and compile shaders
  // -------------------------
  Shader skyboxShader("src/texturedModel.vert", "src/skybox.frag");

  Shader texturedModelWithBonesShader("src/texturedModelWithBones.vert",
                                      "src/texturedModelWithBones.frag");

  Shader simple3dShader("src/simple3d.vert", "src/simple3d.frag");

  Shader simple2dShader("src/simple2d.vert", "src/simple3d.frag");

  Shader groundShader("src/ground.vert", "src/ground.frag");

  Shader grassFieldShader("src/grass.vert", "src/grass.frag");

  GroundPlane ground("resources/grass_ground.png", 10000.0, 500.0);

  GrassField grass = GrassField(100, 100, 0.3);

  // load models
  // -----------
  // tell stb_image.h to flip loaded texture's on the y-axis (before loading
  // model).
  stbi_set_flip_vertically_on_load(false);

  playerHealth = std::make_unique<HealthBar>(HealthBar(
      200.0f, 20.0f, glm::vec2(10.0f, 10.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

  Assimp::Importer knightImporter;
  // knight.emplace(knightImporter,
  // "resources/hollow-knight-hornet/hornet2.glb",
  //                "hornet.008", glm::vec3(5.0f), glm::quat(1.0, 0.0, 0.0,
  //                0.0), glm::vec3(1.0f));
  knight.emplace(knightImporter, "resources/hollow-knight-the-knight.glb",
                 "knight", "Knight_Nail");
  knight->position.x = 3.0;
  knight->model->weaponHitbox->scale = 3.0;
  knight->model->weaponSize *= 2.0;

  Assimp::Importer hornetImporter;
  hornet.emplace(hornetImporter, "resources/hollow-knight-hornet/hornet.gltf",
                 "hornet", "spear nail", glm::vec3(0.0f),
                 glm::quat(1.0, 0.0, 0.0, 0.0), glm::vec3(2.5f));
  hornet->maxHealth = 15;
  hornet->hitbox->scale = 0.7;
  hornet->modelSize *= 0.7f;
  hornet->model->weaponHitbox->scale = 0.7;
  hornet->model->weaponSize *= 0.7;

  Cubemap sky = Cubemap({"resources/sky/right.png", "resources/sky/left.png",
                         "resources/sky/top.png", "resources/sky/bottom.png",
                         "resources/sky/front.png", "resources/sky/back.png"});

  // Assimp::Importer stoneGroundImporter;
  // ModelAnimationAbs stoneGround(stoneGroundImporter,
  //                               "resources/stone_ground_01_a.glb",
  //                               "stoneGround", "");
  // // stoneGround.position.y = 4.0f;

  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwSetMouseButtonCallback(window, mouse_button_callback);

  // draw in wireframe
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (menu_state == MenuType::START_MENU) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

      RenderMenu();

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else if (menu_state == MenuType::LOSE || menu_state == MenuType::WIN) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

      RenderLoseWin(menu_state == MenuType::WIN);

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      // per-framema_engine *pEngine time logic
      // --------------------
      float currentFrame = static_cast<float>(glfwGetTime());
      deltaTime = currentFrame - lastFrame;
      lastFrame = currentFrame;

      if (firstRender == 0) {
        firstRender = currentFrame;
      }

      // input
      // -----
      processInput(window, deltaTime);

      // render
      // ------
      glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);

      knight->updatePosition(deltaTime);

      camera.LookAt = knight->position + glm::vec3(0, 5.0f, 0.0);
      camera.UpdateCameraVectors();

      // view/projection transformations
      glm::mat4 projection =
          glm::perspective(glm::radians(camera.Zoom),
                           (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
      glm::mat4 view = camera.GetViewMatrix();

      glDepthMask(GL_FALSE);
      sky.draw(skyboxShader, view, projection);
      glDepthMask(GL_TRUE);

      grass.draw(grassFieldShader, currentFrame, view, projection);

      // render the loaded model
      glm::mat4 model = glm::mat4(1.0f);
      // Knight position is already updated above
      knight->draw(model, projection, view, texturedModelWithBonesShader,
                   simple3dShader, deltaTime, lastFrame);

      model = glm::mat4(1.0f);
      hornet->updatePosition(deltaTime);
      hornet->draw(model, projection, view, texturedModelWithBonesShader,
                   simple3dShader, deltaTime, lastFrame);

      // ground.Draw(groundShader.ID, view, projection);

      if (currentFrame - firstRender > 3.0f) {
        if (hornetState == HornetState::IDLE &&
            lastFrame > lastHornetAttack + HORNET_ATTACK_COOLDOWN) {
          randomHornetState();
        }
        updateHornetState();
        updateKnightState();
      }

      // Check if knight body hit hornet
      if (hornetState != HornetState::DEAD &&
          lastFrame > DAMAGE_COOLDOWN + knight->lastHit &&
          CheckAABBCollision(knight->position, knight->modelSize,
                             hornet->position, hornet->modelSize)) {
        // knight->position +=
        //     glm::normalize(knight->position - hornet->position) * 1.0f;
        knight->velocity +=
            glm::normalize(knight->position - hornet->position) *
            KNOCKBACK_SPEED;
        playerTakeDamage(1);
        knight->lastHit = lastFrame;
      }

      // Check if knight's nail hit hornet
      if (hornetState != HornetState::DEAD &&
          knightState == KnightState::ATTACKING &&
          lastFrame > DAMAGE_COOLDOWN + hornet->lastHit &&
          CheckAABBCollision(knight->getWeaponPosition(),
                             knight->model->weaponSize, hornet->position,
                             hornet->modelSize)) {
        // std::cout << "HIT " << lastFrame << std::endl;
        // hornet->position +=
        //     glm::normalize(hornet->position - knight->position) * 1.0f;
        hornet->velocity +=
            glm::normalize(hornet->position - knight->position) *
            KNOCKBACK_SPEED;

        hornet->lastHit = lastFrame;
        hornet->health -= 1;
        if (hornet->health == 0) {
          game_over_time = static_cast<float>(glfwGetTime());
          menu_state = MenuType::WIN;
        }
        if (hornet->health == 0) {
          hornetState = HornetState::DEAD;
        }
      }

      // Check if hornet's needle hit knight
      if (hornetState != HornetState::DEAD &&
          lastFrame > DAMAGE_COOLDOWN + knight->lastHit &&
          CheckAABBCollision(knight->position, knight->modelSize,
                             hornet->getWeaponPosition(),
                             hornet->model->weaponSize)) {
        // knight->position +=
        //     glm::normalize(hornet->position - knight->position) * 1.0f;
        knight->velocity +=
            glm::normalize(knight->position - hornet->position) *
            KNOCKBACK_SPEED;
        playerTakeDamage(1);
        knight->lastHit = lastFrame;
      }

      glDisable(GL_DEPTH_TEST);
      simple2dShader.use();
      glm::mat4 uiProjection =
          glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
      playerHealth->draw(uiProjection, simple2dShader.ID);
      glEnable(GL_DEPTH_TEST);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  for (auto &pair : preLoadedSounds) {
    ma_sound_uninit(pair.second.get());
  }
  ma_engine_uninit(&audioEngine);
  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float deltaTime) {
  float currentFrame = static_cast<float>(glfwGetTime());
  if ((menu_state == MenuType::LOSE || menu_state == MenuType::WIN) &&
      currentFrame >= 2.0 + game_over_time) {

    menu_state = MenuType::START_MENU;
    return;
  }

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  glm::vec3 moveDir(0.0f);
  glm::vec3 forwardXY =
      glm::normalize(glm::vec3(camera.Front.x, 0, camera.Front.z));
  glm::vec3 rightXY =
      glm::normalize(glm::vec3(camera.Right.x, 0, camera.Right.z));

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    moveDir += forwardXY;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    moveDir -= forwardXY;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    moveDir -= rightXY;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    moveDir += rightXY;
  }

  if (glm::length(moveDir) > 0.001f) {
    moveDir = glm::normalize(moveDir);
    knight->velocity += moveDir * KNIGHT_ACC * deltaTime;
    // glm::vec3 newPos = knight->position + moveDir * KNIGHT_SPEED * deltaTime;
    // newPos.y = std::max(0.0f, newPos.y);
    // knight->position = newPos;
    // glm::vec3 newCamPos = camera.Position + moveDir * KNIGHT_SPEED *
    // deltaTime; camera.Position = newCamPos;

    // moveDir.y = 0.0f;
    // knight->rotation = glm::rotation(glm::vec3(0, 0, 1), moveDir);

    camera.KnightFront = moveDir;
  }

  camera.LookAt = knight->position;
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  float currentFrame = static_cast<float>(glfwGetTime());
  if ((menu_state == MenuType::LOSE || menu_state == MenuType::WIN) &&
      currentFrame >= 2.0 + game_over_time) {
    menu_state = MenuType::START_MENU;
    return;
  }

  if (imguiIO->WantCaptureMouse) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action,
                                       mods); // Forward to ImGui
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
      lastFrame >= lastKnightStateSet + KNIGHT_ATTACK_DELAY) {
    lastKnightStateSet = lastFrame;
    knightState = KnightState::ATTACKING;
    knight->setAnimation("Knight_NailAction",
                         AnimationRunType::FORWARD_AND_BACKWARD, true);
  }
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  // Calculate offsets based on the change from the last position
  float xoffset = xpos - lastX; // <--- Use local float declaration here
  float yoffset = ypos - lastY;

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset / SCR_WIDTH, yoffset / SCR_HEIGHT);
  knight->rotation = glm::rotation(
      glm::vec3(0, 0, 1),
      glm::normalize(glm::vec3(camera.Front.x, 0.0, camera.Front.z)));
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
