#include "Model.h"
#include <iostream>
#include <algorithm>
#include <limits>

Model::Model(const std::string& path) {
    loadModel(path);
    if (!vertices.empty()) setupBuffers();
}

Model::~Model() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_FixInfacingNormals | // 🔥 КРИТИЧНО ДЛЯ STL
        aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    if (scene->mNumMeshes > 0) processMesh(scene->mMeshes[0]);
    else std::cerr << "Assimp: No meshes found." << std::endl;
}

void Model::processMesh(aiMesh* mesh) {
    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex v;
        v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        v.Normal   = mesh->mNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) indices.push_back(face.mIndices[j]);
    }
    numIndices = static_cast<unsigned int>(indices.size());

    // 🔥 Авто-масштабирование и центрирование
    if (!vertices.empty()) {
        glm::vec3 min_b(std::numeric_limits<float>::max());
        glm::vec3 max_b(std::numeric_limits<float>::lowest());
        for (const auto& v : vertices) { min_b = glm::min(min_b, v.Position); max_b = glm::max(max_b, v.Position); }
        glm::vec3 center = (min_b + max_b) * 0.5f;
        float max_dim = std::max({ max_b.x - min_b.x, max_b.y - min_b.y, max_b.z - min_b.z });
        if (max_dim > 0.001f) {
            float scale = 2.0f / max_dim;
            for (auto& v : vertices) v.Position = (v.Position - center) * scale;
        }
    }
}

void Model::setupBuffers() {
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal)); glEnableVertexAttribArray(1);
}

void Model::Draw(unsigned int shader) const {
    if (VAO == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
}