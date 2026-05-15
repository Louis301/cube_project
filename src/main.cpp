#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_backends/imgui_impl_glfw.h"
#include "imgui_backends/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Model.h"

// ==================== ШЕЙДЕРЫ ====================
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 model; uniform mat4 view; uniform mat4 projection;
out vec3 FragPos; out vec3 Normal;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
})";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos; in vec3 Normal;
out vec4 FragColor;
uniform vec3 lightPos = vec3(2.0, 3.0, 4.0);
uniform vec3 viewPos;
void main() {
    vec3 norm = normalize(Normal);
    if (length(norm) < 0.001) norm = vec3(0.0, 1.0, 0.0); // 🔥 Защита от NaN в STL
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = vec3(0.2);
    vec3 diffuse = diff * vec3(0.6, 0.7, 0.9);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    FragColor = vec4((ambient + diffuse + spec * vec3(0.3)) * vec3(0.4, 0.65, 0.85), 1.0);
})";

// ==================== КАМЕРА И ВВОД ====================
struct Camera {
    glm::vec3 target = glm::vec3(0.0f);
    float distance = 4.0f;
    float yaw = 45.0f, pitch = 20.0f;
    glm::mat4 getView() const {
        return glm::lookAt(
            target + glm::vec3(distance*cos(glm::radians(yaw))*cos(glm::radians(pitch)),
                               distance*sin(glm::radians(pitch)),
                               distance*sin(glm::radians(yaw))*cos(glm::radians(pitch))),
            target, glm::vec3(0,1,0));
    }
    glm::vec3 getPos() const {
        return target + glm::vec3(distance*cos(glm::radians(yaw))*cos(glm::radians(pitch)),
                                  distance*sin(glm::radians(pitch)),
                                  distance*sin(glm::radians(yaw))*cos(glm::radians(pitch)));
    }
};

struct InputState { Camera cam; double lastX=0, lastY=0; bool firstMouse=true; };

// 🔥 Обычные C-функции + форвардинг в ImGui
void cursor_cb(GLFWwindow* w, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(w, x, y);
    auto* s = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
        if (s->firstMouse) { s->lastX = x; s->lastY = y; s->firstMouse = false; }
        float dx = static_cast<float>(x - s->lastX);
        float dy = static_cast<float>(y - s->lastY);
        s->cam.yaw += dx * 0.4f;
        s->cam.pitch += dy * 0.4f;
        s->cam.pitch = glm::clamp(s->cam.pitch, -89.0f, 89.0f);
        s->lastX = x; s->lastY = y;
    } else s->firstMouse = true;
}

void scroll_cb(GLFWwindow* w, double, double yo) {
    ImGui_ImplGlfw_ScrollCallback(w, 0.0, yo);
    auto* s = static_cast<InputState*>(glfwGetWindowUserPointer(w));
    s->cam.distance -= static_cast<float>(yo) * 0.5f;
    s->cam.distance = glm::clamp(s->cam.distance, 0.5f, 50.0f);
}

// ==================== ВСПОМОГАТЕЛЬНЫЕ ====================
unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int id = glCreateShader(type); glShaderSource(id, 1, &src, nullptr); glCompileShader(id);
    int s; char l[512]; glGetShaderiv(id, GL_COMPILE_STATUS, &s);
    if (!s) { glGetShaderInfoLog(id, 512, nullptr, l); std::cerr << "Shader Error: " << l << '\n'; }
    return id;
}
unsigned int createProgram(const char* v, const char* f) {
    unsigned int p = glCreateProgram();
    glAttachShader(p, compileShader(GL_VERTEX_SHADER, v));
    glAttachShader(p, compileShader(GL_FRAGMENT_SHADER, f));
    glLinkProgram(p); return p;
}

// ==================== MAIN ====================
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "STL Viewer", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE); // 🔥 Обязательно для glfwGetMouseButton

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // 1. Привязка состояния ДО колбэков
    InputState input;
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, cursor_cb);
    glfwSetScrollCallback(window, scroll_cb);

    // 2. ImGui (install_callbacks=false!)
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    unsigned int shader = createProgram(vertexShaderSource, fragmentShaderSource);
    Model model("assets/model.stl"); // Положите файл сюда

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // 🔥 Отключаем, т.к. STL часто имеет смешанный winding order

    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGui::Begin("Controls");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Distance", &input.cam.distance, 0.5f, 20.0f);
        if (ImGui::Button("Reset")) { input.cam = { glm::vec3(0), 4.0f, 45.0f, 20.0f }; }
        ImGui::End();
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

        model.Draw(shader);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window); glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwTerminate(); return 0;
}