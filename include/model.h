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

class alignas(16) Light {
public:
    Light(vec4 _center, float _a_len, float _b_len, vec4 _norm, vec4 _a_vec,vec4 _color, float _power);
    ~Light();
private:
    vec4 center;
    vec4 norm;
    vec4 a_vec;
    vec4 color;
    float a_len;
    float b_len;
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
            position[0 + i] = a.position[i];
            position[3 + i] = b.position[i];
            position[6 + i] = c.position[i];

            normal[0 + i] = a.normal[i];
            normal[3 + i] = b.normal[i];
            normal[6 + i] = c.normal[i];

            if (i < 2) { // texCoords 只有 2 维
                uv[0 + i] = a.texCoords[i];
                uv[2 + i] = b.texCoords[i];
                uv[4 + i] = c.texCoords[i];
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