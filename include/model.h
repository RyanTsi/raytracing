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
#include "material.h"

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

class Light {
public:
    Light(vec3 _center, float _width, float _length, vec3 _norm, vec3 _color, float _power);
    void set(unsigned int id, Shader &shader);
    ~Light();
private:
    vec3 center;
    float width;
    float length;
    vec3 norm;
    vec3 color;
    float power;
};

class Mesh {
private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // TODO: Texture
    // std::vector<Texture> textures;
    const Material *material;
    Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, const Material *_material);
    void draw(Shader &shader);
    ~Mesh();
};

class Scene {
public:
    Scene(const char *path);
    ~Scene();
    void draw(Shader &shader);
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Light> lights;
    void addLight(Light light);
private:
    std::string directory;
    std::vector<Texture> textures_loaded;
    void loadScene(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};

unsigned int TextureFromFile(const char *path, const std::string &directory);

