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

#include <iostream>
#include <optional>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::optional<ModelAnimationAbs> knight;

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

  // build and compile shaders
  // -------------------------
  Shader texturedModelShader("src/texturedModel.vert",
                             "src/texturedModel.frag");

  Shader simple3dShader("src/simple3d.vert", "src/simple3d.frag");

  Assimp::Importer importer;

  Plane ground(1.0f);
  ground.position = glm::vec3(0.0f, 0.0f, 0.0f);
  ground.rotation = glm::vec3(0.0f);

  // load models
  // -----------
  // tell stb_image.h to flip loaded texture's on the y-axis (before loading
  // model).
  stbi_set_flip_vertically_on_load(false);
  knight = ModelAnimationAbs(
      importer,
      "resources/hollow-knight-the-knight/hollow-knight-the-knight-v3.glb");

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
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    // model =
    //     glm::rotate(model, (float)(M_PI / 2.0), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

    knight->draw(model, texturedModelShader, deltaTime);

    camera.LookAt = knight->position;

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}

void updateCameraFollow() {}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  //   camera.ProcessKeyboard(FORWARD, deltaTime);
  // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  //   camera.ProcessKeyboard(BACKWARD, deltaTime);
  // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  //   camera.ProcessKeyboard(LEFT, deltaTime);
  // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  //   camera.ProcessKeyboard(RIGHT, deltaTime);

  float modelSpeed = 1;

  glm::vec3 moveDir(0.0f);
  glm::vec3 forwardXY =
      glm::normalize(glm::vec3(camera.Front.x, camera.Front.y, 0));
  glm::vec3 rightXY =
      glm::normalize(glm::vec3(camera.Right.x, camera.Right.y, 0.0f));

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    moveDir += forwardXY;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    moveDir -= forwardXY;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    moveDir -= rightXY;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    moveDir += rightXY;

  if (glm::length(moveDir) > 0.0f) {
    moveDir = glm::normalize(moveDir);
    knight->position += moveDir * modelSpeed * deltaTime;
  }

  // update camera to follow
  updateCameraFollow();
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
    knight->setAnimation("Knight_NailAction");
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
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
