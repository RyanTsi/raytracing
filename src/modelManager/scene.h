#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.h"
#include "camera.h"
#include "light.h"
#include "shader.h"
#include "material.h"
#include "BVH.h"

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
private:
    std::string directory;
    std::vector<Mesh> meshes;
    std::vector<Light> lights;
    std::vector<Material> materials;
    void loadScene(const std::string &path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    void setFrame();
    GLuint VAO, VBO;
    Camera camera;
    std::vector<TriangleData> trianglesBuffer;
    std::vector<float> lightsBuffer;
    std::vector<float> materialsBuffer;
    BVH bvh;
    bool isUpdate;
    void setCamera(Shader &shader);
    void setBuffer(Shader &shader);
public:
    Scene(const char *path);
    void draw(Shader &shader);
    // void setInShaderD(Shader &shader);
    // void setInShaderS(Shader &shader);
    void addLight(Light light);
    void updateBuffer();
    ~Scene();
};