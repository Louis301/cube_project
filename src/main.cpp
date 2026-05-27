#define IMGUI_IMPL_OPENGL_LOADER_GLAD // Если остался в проекте, можно убрать
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Model.h"
#include <algorithm>
#include <imgui.h>
#include "imgui_backends/imgui_impl_glfw.h"
#include "imgui_backends/imgui_impl_opengl3.h"

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 Normal;
out vec3 FragPos;
void main() {
FragPos = vec3(model * vec4(aPos, 1.0));
Normal = mat3(transpose(inverse(model))) * aNormal;
gl_Position = projection * view * vec4(FragPos, 1.0);
})";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;
out vec4 FragColor;
// Фиксированное направление света (изометрия)
uniform vec3 lightDir = vec3(0.5, 0.8, 0.3);
void main() {
vec3 norm = normalize(Normal);
if (length(norm) < 0.001) norm = vec3(0.0, 1.0, 0.0); // Fallback для плохих STL
float diff = max(dot(norm, normalize(lightDir)), 0.0);
vec3 ambient = vec3(0.25);
vec3 color = vec3(0.35, 0.60, 0.85); // Приятный синий оттенок
FragColor = vec4((ambient + diff * 0.75) * color, 1.0);
})";

unsigned int compileShader(unsigned int type, const char* src) 
{
unsigned int id = glCreateShader(type);
glShaderSource(id, 1, &src, nullptr);
glCompileShader(id);
int s; char l[512];
glGetShaderiv(id, GL_COMPILE_STATUS, &s);
if (!s) { glGetShaderInfoLog(id, 512, nullptr, l); std::cerr << "Shader Error: " << l << '\n'; }
return id;
}

unsigned int createProgram(const char* v, const char* f) 
{
unsigned int p = glCreateProgram();
glAttachShader(p, compileShader(GL_VERTEX_SHADER, v));
glAttachShader(p, compileShader(GL_FRAGMENT_SHADER, f));
glLinkProgram(p);
return p;
}

struct Camera {
glm::vec3 target = glm::vec3(0.0f);
float distance = 4.0f;
float yaw = 45.0f, pitch = 20.0f;

glm::mat4 getView() const {
return glm::lookAt(
target + glm::vec3(
distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
distance * sin(glm::radians(pitch)),
distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch))
), target, glm::vec3(0, 1, 0)
);
}

glm::vec3 getPos() const {
return target + glm::vec3(
distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
distance * sin(glm::radians(pitch)),
distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch))
);
}
};

struct InputState { 
Camera cam; 
double lastX=0, lastY=0; 
bool firstMouse=true; 
};

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
if (ImGui::GetIO().WantCaptureMouse) return;
auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(window));
if (!state) return;
state->cam.distance -= static_cast<float>(yoffset) * 0.5f;
state->cam.distance = glm::clamp(state->cam.distance, 0.5f, 50.0f);
}

void cursor_cb(GLFWwindow* window, double xpos, double ypos) {
ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(window));
if (!state) return;
if (ImGui::GetIO().WantCaptureMouse) return;
if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
if (state->firstMouse) {
state->lastX = xpos; state->lastY = ypos; state->firstMouse = false;
}
float dx = static_cast<float>(xpos - state->lastX);
float dy = static_cast<float>(ypos - state->lastY);
state->cam.yaw += dx * 0.4f;
state->cam.pitch += dy * 0.4f;
state->cam.pitch = glm::clamp(state->cam.pitch, -89.0f, 89.0f);
state->lastX = xpos; state->lastY = ypos;
} else {
state->firstMouse = true;
}
}

void scroll_cb(GLFWwindow* window, double, double yoffset) {
ImGui_ImplGlfw_ScrollCallback(window, 0.0, yoffset);
auto* state = static_cast<InputState*>(glfwGetWindowUserPointer(window));
if (!state) return;
state->cam.distance -= static_cast<float>(yoffset) * 0.5f;
state->cam.distance = glm::clamp(state->cam.distance, 0.5f, 50.0f);
}

int main() {
if (!glfwInit()) return -1;
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
GLFWwindow* window = glfwCreateWindow(1280, 720, "STL Isometric Viewer", nullptr, nullptr);
if (!window) return -1;
glfwMakeContextCurrent(window);
glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
InputState input;
glfwSetWindowUserPointer(window, &input);
glfwSetCursorPosCallback(window, cursor_cb);
glfwSetScrollCallback(window, scroll_cb);
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGui::StyleColorsDark();
ImGui_ImplGlfw_InitForOpenGL(window, false);
ImGui_ImplOpenGL3_Init("#version 330 core");
unsigned int shader = createProgram(vertexShaderSource, fragmentShaderSource);
glUseProgram(shader);
Model model("assets/model3.stl");
glEnable(GL_DEPTH_TEST);
glDisable(GL_CULL_FACE);
while (!glfwWindowShouldClose(window)) {
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplGlfw_NewFrame();
ImGui::NewFrame();
ImGui::Render();
glClearColor(0.12f, 0.15f, 0.2f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glm::mat4 view = input.cam.getView();
glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
glm::mat4 modelMat = glm::mat4(1.0f);
glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, glm::value_ptr(input.cam.getPos()));
model.Draw();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
glfwSwapBuffers(window);
glfwPollEvents();
}
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplGlfw_Shutdown();
ImGui::DestroyContext();
glfwTerminate();
return 0;
}