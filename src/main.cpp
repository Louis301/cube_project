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

// ==================== МИНИ-ШЕЙДЕРЫ ====================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 FragPos;
    out vec3 Normal;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    out vec4 FragColor;
    uniform vec3 lightPos = vec3(2.0, 3.0, 4.0);
    uniform vec3 viewPos;
    void main() {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 ambient = vec3(0.15);
        vec3 diffuse = diff * vec3(0.7, 0.8, 1.0);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * vec3(0.3);
        FragColor = vec4((ambient + diffuse + specular) * vec3(0.4, 0.65, 0.85), 1.0);
    }
)";

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "SHADER COMPILE ERROR:\n" << infoLog << std::endl;
    }
    return id;
}

unsigned int createProgram(const char* vertex, const char* fragment) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertex);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ==================== КАМЕРА ====================
struct Camera {
    glm::vec3 target = glm::vec3(0.0f);
    float distance = 5.0f;
    float yaw = 45.0f;
    float pitch = 20.0f;
    
    glm::mat4 getView() const {
        float r = distance;
        float camX = target.x + r * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        float camY = target.y + r * sin(glm::radians(pitch));
        float camZ = target.z + r * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::lookAt(glm::vec3(camX, camY, camZ), target, glm::vec3(0,1,0));
    }
    glm::vec3 getPos() const { return getView() * glm::vec4(0,0,1,1); } // упрощённо для viewPos
};

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "M3D Viewer", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // === Инициализация ImGui ===
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // === Шейдеры ===
    unsigned int shaderProg = createProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProg);

    // === Куб (VAO/VBO/EBO) ===
    float vertices[] = {
        // pos              // normal
        -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
         0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
         0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
        -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
        -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
         0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
         0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
        -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f
    };
    unsigned int indices[] = {
        0,1,2, 2,3,0,  4,5,6, 6,7,4,  8,9,10, 10,11,8,
        12,13,14, 14,15,12,  16,17,18, 18,19,16,  20,21,22, 22,23,20
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);

    // === Обработка мыши для камеры ===
    Camera cam;
    double lastX = 0, lastY = 0;
    bool firstMouse = true;

    glfwSetCursorPosCallback(window, [&](GLFWwindow*, double xpos, double ypos) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
            float dx = static_cast<float>(xpos) - static_cast<float>(lastX);
            float dy = static_cast<float>(ypos) - static_cast<float>(lastY);
            cam.yaw   += dx * 0.5f;
            cam.pitch += dy * 0.5f;
            cam.pitch  = glm::clamp(cam.pitch, -89.0f, 89.0f);
            lastX = xpos; lastY = ypos;
        } else {
            firstMouse = true;
        }
    });

    glfwSetScrollCallback(window, [&](GLFWwindow*, double xoffset, double yoffset) {
        cam.distance -= static_cast<float>(yoffset) * 0.5f;
        cam.distance = glm::clamp(cam.distance, 1.0f, 50.0f);
    });

    // === Цикл рендера ===
    while (!glfwWindowShouldClose(window)) {
        // 1. ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Viewer Controls");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("Distance", &cam.distance, 1.0f, 20.0f);
        ImGui::SliderFloat("Yaw", &cam.yaw, 0.0f, 360.0f);
        ImGui::SliderFloat("Pitch", &cam.pitch, -89.0f, 89.0f);
        if (ImGui::Button("Reset Camera")) {
            cam.distance = 5.0f; cam.yaw = 45.0f; cam.pitch = 20.0f;
        }
        ImGui::End();

        ImGui::Render();

        // 2. Рендер OpenGL
        glClearColor(0.12f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProg);
        glm::mat4 view = cam.getView();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f); // Здесь потом будет загрузка .m3d

        glUniformMatrix4fv(glGetUniformLocation(shaderProg, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProg, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shaderProg, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProg, "viewPos"), 1, glm::value_ptr(glm::vec3(cam.distance * sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch)),
                                                                                                cam.distance * sin(glm::radians(cam.pitch)),
                                                                                                cam.distance * cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch)))));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // 3. Отрисовка ImGui поверх
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}