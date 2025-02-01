#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "shaderManager.h"
#include <iostream>
#include <stb_image.h>

using glm::vec3;
using glm::vec2;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    Vertex(vec3 _position, vec3 _normal, vec2 _texCoords) {
        this->position  = _position;
        this->normal    = _normal;
        this->texCoords = _texCoords;
    }
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, std::vector<Texture> _textures);
    void draw(Shader &shader);
    ~Mesh();
};

class Model {
public:
    Model(const char *path);
    ~Model();
    void draw(Shader &shader);   
    std::vector<Mesh> meshes;
private:
    std::string directory;
    std::vector<Texture> textures_loaded;
    void loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};

unsigned int TextureFromFile(const char *path, const std::string &directory);