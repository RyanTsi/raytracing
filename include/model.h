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
#include "tools.h"
#include "camera.h"

using glm::vec3;
using glm::vec2;

struct Vertex {
    vec4 position;
    vec4 normal;
    vec2 texCoords;
    Vertex(glm::vec4 _position, glm::vec4 _normal, glm::vec2 _texCoords)
        : position(_position), normal(_normal), texCoords(_texCoords) {}
    Vertex() {};
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Light {
public:
    Light(vec4 _center, float _width, float _length, vec4 _norm, vec4 _color, float _power);
    ~Light();
private:
    vec4 center;
    float width;
    float length;
    vec4 norm;
    vec4 color;
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

struct TriangleData {
    float position[9];
    float normal[9];
    float uv[6];
    GLuint materialIdx;
    TriangleData() {}
    TriangleData(Vertex &a, Vertex &b, Vertex &c, GLuint matIdx = 0) 
        : materialIdx(matIdx) {
        for (int i = 0; i < 3; i ++) {
            position[i * 3 + 0] = a.position[i];
            position[i * 3 + 1] = b.position[i];
            position[i * 3 + 2] = c.position[i];

            normal[i * 3 + 0] = a.normal[i];
            normal[i * 3 + 1] = b.normal[i];
            normal[i * 3 + 2] = c.normal[i];

            if (i < 2) { // texCoords 只有 2 维
                uv[i * 3 + 0] = a.texCoords[i];
                uv[i * 3 + 1] = b.texCoords[i];
                uv[i * 3 + 2] = c.texCoords[i];
            }
        }
    }
};

class Scene {
public:
    Scene(const char *path);
    void draw(Shader &shader);
    void setInShaderD(Shader &shader);
    void setInShaderS(Shader &shader);
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Light> lights;
    std::vector<TriangleData> trianglesBuffer;
    Camera camera;
    void addLight(Light light);
    ~Scene();
private:
    unsigned int VAO, VBO, EBO;
    bool isInit;
    void setFrame();
    void setCamera();
    void setMesh();
    std::string directory;
    std::vector<Texture> textures_loaded;
    void loadScene(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};

unsigned int TextureFromFile(const char *path, const std::string &directory);