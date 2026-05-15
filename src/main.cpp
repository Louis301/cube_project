#define IMGUI_IMPL_OPENGL_LOADER_GLAD // Если остался в проекте, можно убрать
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int s; char l[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &s);
    if (!s) { glGetShaderInfoLog(id, 512, nullptr, l); std::cerr << "Shader Error: " << l << '\n'; }
    return id;
}

unsigned int createProgram(const char* v, const char* f) {
    unsigned int p = glCreateProgram();
    glAttachShader(p, compileShader(GL_VERTEX_SHADER, v));
    glAttachShader(p, compileShader(GL_FRAGMENT_SHADER, f));
    glLinkProgram(p);
    return p;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "STL Isometric Viewer", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    unsigned int shader = createProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shader);

    Model model("assets/model.stl"); // Положите файл сюда

    // 🔥 Фиксированная изометрическая проекция
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(3.0f, 2.5f, 3.0f), // Позиция камеры (изометрический угол)
        glm::vec3(0.0f, 0.0f, 0.0f), // Центр сцены
        glm::vec3(0.0f, 1.0f, 0.0f)  // Вектор вверх
    );
    glm::mat4 modelMat = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // STL часто имеет смешанный winding order

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.12f, 0.15f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model.Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}