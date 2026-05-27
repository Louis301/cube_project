#pragma once
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Model {
public:
    Model(const std::string& path);
    void Draw() const;
    ~Model();

private:
    void loadModel(const std::string& path);
    void processMesh(aiMesh* mesh);
    void setupBuffers();

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int numIndices = 0;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};