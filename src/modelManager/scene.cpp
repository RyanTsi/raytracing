#include "scene.h"

Scene::Scene(const char *path) {
    isUpdate = true;
    loadScene(path);
    setFrame();
    camera = Camera(0, 0, 10);
    addLight(Light(vec3(20, 5, 30), 20, 20, vec3(-1, -0.2, -0.6), vec3(0.2, -1, 0), vec3(1.0, 1.0, 1.0), 15.0));
}

Scene::~Scene() {}

void Scene::loadScene(const std::string &path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_FlipUVs);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)  {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    for(unsigned int i = 0; i < scene->mNumMaterials; i ++) {
        const aiMaterial *material = scene->mMaterials[i];
        materials.push_back(Material(material));
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Scene::processNode(aiNode *node, const aiScene *scene) {
    for(unsigned int i = 0; i < node->mNumMeshes; i ++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for(unsigned int i = 0; i < node->mNumChildren; i ++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Scene::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material *material;
    for(int i = 0; i < mesh->mNumVertices; i ++) {
        Vertex vertex(
            vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
            vec3(mesh->mNormals[i].x , mesh->mNormals[i].y , mesh->mNormals[i].z),
            vec2(0.0f)
        );
        // TODO: uv coords 0-8 ?
        if(mesh->mTextureCoords[0]) {
            vertex.texCoords = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        vertices.push_back(vertex);
    }
    for(int i = 0; i < mesh->mNumFaces; i ++) {
        aiFace face = mesh->mFaces[i];
        for(int j = 0; j < face.mNumIndices; j ++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    material = &materials[mesh->mMaterialIndex];
    return Mesh(vertices, indices, material);
}

void Scene::setFrame() {
    float quad[12] = {
        -1.0f,  1.0f, // 左上角
        -1.0f, -1.0f, // 左下角
         1.0f, -1.0f, // 右下角
        -1.0f,  1.0f, // 左上角
         1.0f, -1.0f, // 右下角
         1.0f,  1.0f  // 右上角
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Scene::updateBuffer() {
    trianglesBuffer.clear();
    lightsBuffer.clear();
    materialsBuffer.clear();
    std::vector<Triangle> trs;
    for(auto mesh : meshes) {
        for(unsigned int i = 0; i < mesh.indices.size(); i += 3) {
            trianglesBuffer.push_back(
                TriangleData(mesh.vertices[mesh.indices[i]],
                    mesh.vertices[mesh.indices[i + 1]],
                    mesh.vertices[mesh.indices[i + 2]],
                    mesh.material - &materials[0]
                ));
            trs.push_back(
                Triangle(mesh.vertices[mesh.indices[i]].position,
                mesh.vertices[mesh.indices[i + 1]].position,
                mesh.vertices[mesh.indices[i + 2]].position,
                i / 3
            ));
        }
    }
    bvh = BVH(trs);
    for(auto light : lights) {
        std::vector<float> temp = light.getData();
        lightsBuffer.insert(lightsBuffer.end(), temp.begin(), temp.end());
    }
    for(auto material : materials) {
        std::vector<float> temp = material.getData();
        materialsBuffer.insert(materialsBuffer.end(), temp.begin(), temp.end());
    }
    isUpdate = false;
}

void Scene::addLight(Light light) {
    lights.push_back(light);
}

void Scene::draw(Shader &shader) {
    if(isUpdate) {
        updateBuffer();
        setBuffer(shader);
    }
    setCamera(shader);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::setCamera(Shader &shader) {
    shader.setVec3("uCamera.position", camera.position);
    shader.setVec3("uCamera.front", camera.front);
    shader.setVec3("uCamera.up", camera.upDirection);
    shader.setFloat("uCamera.fov", camera.eyeFov);
    shader.setFloat("uCamera.aspectRatio", camera.aspectRatio);
}

void Scene::setBuffer(Shader &shader) {
    shader.setInt("SCREEN_WIDTH", SCREEN_WIDTH);
    shader.setInt("SCREEN_HEIGHT", SCREEN_HEIGHT);
    shader.addSSBO("lightBuffer", lightsBuffer.data(), lightsBuffer.size() * sizeof(float), 1);    
    shader.addSSBO("materialBuffer", materialsBuffer.data(), materialsBuffer.size() * sizeof(float), 2);
    shader.addSSBO("triangleBuffer", trianglesBuffer.data(), trianglesBuffer.size() * sizeof(TriangleData), 3);
}