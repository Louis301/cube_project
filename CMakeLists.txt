cmake_minimum_required(VERSION 3.20)

# ..надо заменить на сист. переменную
set(CMAKE_TOOLCHAIN_FILE "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(cube LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES 
  "src/*.cpp"
#   "src/*.h"
  "src/imgui_backends/*.cpp"
#   "src/imgui_backends/*.h"
)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
  glfw
  glad::glad
  glm::glm
  imgui::imgui
  assimp::assimp
  opengl32
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
endif()