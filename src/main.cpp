#include "glm/geometric.hpp"
#include "learnopengl/model_animation.h"
#include "learnopengl/plane.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/model.h>
#include <learnopengl/model_animation_abstraction.h>
#include <learnopengl/shader_m.h>

#include <learnopengl/animator.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <optional>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

ma_engine engine;

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

enum HornetState {
  LUNGE_WAIT,
  LUNGE,
  DASH,
  IDLE,
};

HornetState hornetState = IDLE;
float lastHornetStateSet = 0.0f;
float lastHornetAttack = 0.0f;

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
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwSetMouseButtonCallback(window, mouse_button_callback);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  ma_result result;

  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    return -1;
  }

  // build and compile shaders
  // -------------------------
  Shader texturedModelShader("src/texturedModel.vert",
                             "src/texturedModel.frag");

  Shader simple3dShader("src/simple3d.vert", "src/simple3d.frag");

  Plane ground(1.0f);
  ground.position = glm::vec3(0.0f, 0.0f, 0.0f);
  ground.rotation = glm::vec3(0.0f);

  // load models
  // -----------
  // tell stb_image.h to flip loaded texture's on the y-axis (before loading
  // model).
  stbi_set_flip_vertically_on_load(false);

  Assimp::Importer knightImporter;
  knight.emplace(knightImporter, "resources/hollow-knight-the-knight.glb",
                 "Knight_Nail");

  Assimp::Importer hornetImporter;
  hornet.emplace(hornetImporter,
                 "resources/hollow-knight-hornet/source/hornet-v2.glb",
                 "hornet.008");
  hornet->position.x = 3.0f;
  hornet->scale = 2.5f;

  // draw in wireframe
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // per-frame time logic
    // --------------------
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    // -----
    processInput(window);

    // glfwSetCursorPos(window, SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
    // lastX = SCR_WIDTH / 2.0f;
    // lastY = SCR_HEIGHT / 2.0f;

    // render
    // ------
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // don't forget to enable shader before setting uniforms
    texturedModelShader.use();

    // view/projection transformations
    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    texturedModelShader.setMat4("projection", projection);
    texturedModelShader.setMat4("view", view);

    simple3dShader.use();
    simple3dShader.setMat4("projection", projection);
    simple3dShader.setMat4("view", view);
    ground.Draw(simple3dShader);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    knight->draw(model, texturedModelShader, simple3dShader, deltaTime,
                 lastFrame);

    model = glm::mat4(1.0f);
    hornet->draw(model, texturedModelShader, simple3dShader, deltaTime,
                 lastFrame);

    if (hornetState == IDLE &&
        lastFrame > lastHornetAttack + HORNET_ATTACK_COOLDOWN) {
      // int action = (std::rand() % 3);
      // hornetState = static_cast<HornetState>(action);
      hornetState = LUNGE_WAIT;

      glm::mat4 lookAtMatrix = glm::lookAt(hornet->position, knight->position,
                                           glm::vec3(0.0f, 1.0f, 0.0f));
      glm::mat3 rotationMatrix = glm::mat3(glm::transpose(lookAtMatrix));

      switch (hornetState) {
      case LUNGE_WAIT: {
        hornet->rotation = glm::quat_cast(rotationMatrix);
        ma_engine_play_sound(&engine, "resources/audio/shaw.mp3", NULL);
        lastHornetStateSet = lastFrame;
        break;
      }
      case LUNGE: {
        break;
      }
      case DASH: {
        hornet->rotation = glm::quat_cast(rotationMatrix);
        hornet->setAnimation("point_forward", true);
        break;
      }
      case IDLE:
        break;
      }
      lastHornetAttack = lastFrame;
    }

    switch (hornetState) {
    case LUNGE_WAIT: {
      if (lastFrame > lastHornetStateSet + 1.0f) {
        hornetState = LUNGE;
        hornet->setAnimation("lunge", true);
        lastHornetStateSet = lastFrame;
      }
      break;
    }
    case LUNGE: {
      if (hornet->animator.isOver()) {
        hornetState = IDLE;
      }
      break;
    }
    case DASH: {
      hornet->position -= hornet->getFront() * HORNET_DASH_SPEED * deltaTime;
      break;
    }
    case IDLE:
      break;
    }

    // Check if knight body hit hornet
    if (CheckAABBCollision(knight->position, knight->modelSize,
                           hornet->position, hornet->modelSize) &&
        lastFrame > DAMAGE_COOLDOWN + knight->lastHit) {
      knight->position +=
          glm::normalize(knight->position - hornet->position) * 1.0f;
      std::cout << "COLLISIONSS " << lastFrame << std::endl;
      // std::cout << "knight: width " << knight->width << " height "
      //           << knight->height << std::endl;
      // std::cout << "hornet: width " << hornet->width << " height "
      //           << hornet->height << std::endl;
      knight->lastHit = lastFrame;
    }

    // Check if knight's nail hit hornet
    if (CheckAABBCollision(knight->getWeaponPosition(lastFrame),
                           knight->model->weaponSize, hornet->position,
                           hornet->modelSize) &&
        lastFrame > DAMAGE_COOLDOWN + hornet->lastHit) {
      hornet->position +=
          glm::normalize(hornet->position - knight->position) * 1.0f;
      std::cout << "COLLISIONSS " << lastFrame << std::endl;
      // std::cout << "knight: width " << knight->width << " height "
      //           << knight->height << std::endl;
      // std::cout << "hornet: width " << hornet->width << " height "
      //           << hornet->height << std::endl;
      hornet->lastHit = lastFrame;
    }

    // camera.LookAt = knight->position;

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  ma_engine_uninit(&engine);
  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
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
    glm::vec3 newPos = knight->position + moveDir * KNIGHT_SPEED * deltaTime;
    newPos.y = std::max(0.0f, newPos.y);
    knight->position = newPos;

    moveDir.y = 0.0f;
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
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    glm::vec3 pos = knight->position + knight->getFront() * 1.0f;
    float posX = pos.x;
    float posY = pos.y;
    // // collision x-axis?
    // bool collisionX = hornet->position.x + hornet->widthX >= posX &&
    //                   posX + knight->widthX >= hornet->position.x;
    // // collision y-axis?
    // bool collisionY = hornet->position.y + hornet->widthZ >= posY &&
    //                   posY + knight->widthZ >= hornet->position.y;

    // std::cout << posX << " hornet x: " << hornet->position.x << " width "
    //           << knight->width << std::endl;
    // std::cout << posY << " hornet x: " << hornet->position.y << " height "
    //           << knight->height << std::endl;
    // std::cout << collisionX << " " << collisionY << std::endl;
    // collision only if on both axes
    // if (collisionX && collisionY) {
    //   hornet->lastHit = lastFrame;
    // }
    knight->setAnimation("Knight_NailAction", true);
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
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

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
