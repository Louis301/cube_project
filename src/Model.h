#pragma once
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Структура вершины (позиция + нормаль)
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Model {
public:
    Model(const std::string& path);
    void Draw(unsigned int shader) const;
    ~Model();

private:
    void loadModel(const std::string& path);
    void setupMesh();
    void processMesh(aiMesh* mesh);

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int numIndices = 0;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};