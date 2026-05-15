cmake_minimum_required(VERSION 3.20)

# ..надо заменить на сист. переменную
set(CMAKE_TOOLCHAIN_FILE "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")


project(cub LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Поиск зависимостей (vcpkg предоставляет namespaced targets)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Сборка исполняемого файла
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES}
    src/imgui_backends/imgui_impl_glfw.cpp
    src/imgui_backends/imgui_impl_opengl3_loader.h
    src/Model.cpp)

# Линковка
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
    imgui::imgui
    assimp::assimp
    opengl32  # Системная библиотека Windows
)

# MSVC: строгие предупреждения + UTF-8
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
endif()