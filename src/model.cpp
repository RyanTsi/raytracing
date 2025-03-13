#include "model.h"

Light::Light(vec4 _center, float _a_len, float _b_len, vec4 _norm, vec4 _a_vec,vec4 _color, float _power) {
    center = _center;
    a_len  = _a_len;
    b_len  = _b_len;
    a_vec  = glm::normalize(_a_vec);
    norm   = glm::normalize(_norm);
    color  = glm::normalize(_color);
    power  = _power;
}

Light::~Light() {}

Mesh::Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, const Material *_material) {
    this->vertices = _vertices;
    this->indices  = _indices;
    this->material = _material;
    setupMesh();
}

Mesh::~Mesh() {}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    // set VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    // set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    // set VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoords)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // unbind
    // glBindBuffer(GL_ARRAY_BUFFER, 0); 
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
}

void Mesh::draw(Shader &shader)  {
    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

Scene::Scene(const char *path) {
    isInit = false;
    loadScene(path);
    addLight(Light(vec4(20, 5, 30, 0), 20, 20, vec4(-1, -0.2, -0.6, 0), vec4(0.2, -1, 0, 0), vec4(1.0, 1.0, 1.0, 1.0), 15.0));
    // addLight(Light(vec4(0, 0, -100, 0), 100, 100, vec4(0, 0, 1, 0), vec4(1, 0, 0, 0), vec4(1.0, 1.0, 1.0, 0), 50000.0));
    // addLight(Light(vec4(0, 0, 100, 0), 100, 100, vec4(0, 0, -1, 0), vec4(1, 0, 0, 0), vec4(1.0, 1.0, 1.0, 0), 50000.0));
    setFrame();
    setCamera();
    setMesh();
}

void Scene::setCamera() {
    camera = Camera(0, 0, 10);
}

void Scene::setMesh() {
    for(auto mesh : meshes) {
        for(unsigned int i = 0; i < mesh.indices.size(); i += 3) {
            trianglesBuffer.push_back(
                TriangleData(mesh.vertices[mesh.indices[i]],
                         mesh.vertices[mesh.indices[i + 1]],
                         mesh.vertices[mesh.indices[i + 2]],
                         mesh.material - &materials[0]
                ));
        }
    }
}

void Scene::setInShaderD(Shader &shader) {
    shader.setVec3("uCamera.position", camera.position);
    shader.setVec3("uCamera.front", camera.front);
    shader.setVec3("uCamera.up", camera.upDirection);
    shader.setFloat("uCamera.fov", camera.eyeFov);
    shader.setFloat("uCamera.aspectRatio", camera.aspectRatio);
}

void Scene::setInShaderS(Shader &shader) {
    shader.setInt("SCREEN_WIDTH", SCREEN_WIDTH);
    shader.setInt("SCREEN_HEIGHT", SCREEN_HEIGHT);
    shader.addSSBO("lightBuffer", lights.data(), lights.size() * sizeof(Light), 1);    
    shader.addSSBO("materialBuffer", materials.data(), materials.size() * sizeof(Material), 2);
    shader.addSSBO("triangleBuffer", trianglesBuffer.data(), trianglesBuffer.size() * sizeof(TriangleData), 3);
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

void Scene::loadScene(std::string path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_FlipUVs);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)  {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    // std::map<unsigned int, unsigned int> mp;
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
            vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 0),
            vec4(mesh->mNormals[i].x , mesh->mNormals[i].y , mesh->mNormals[i].z,  0),
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

std::vector<Texture> Scene::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++) {
            if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true; 
                break;
            }
        }
        if(!skip) {   // 如果纹理还没有被加载，则加载它
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture); // 添加到已加载的纹理中
        }
    }
    return textures;
}

void Scene::draw(Shader &shader) {
    if(!isInit) setInShaderS(shader), isInit = true;
    setInShaderD(shader);
    // for(int i = 0; i < meshes.size(); i ++) {
    //     meshes[i].draw(shader);
    // }
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::addLight(Light light) {
    lights.push_back(light);
}

Scene::~Scene() {}

unsigned int TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void initShaer(Shader &shader) {
    shader.setInt("SCREEN_WIDTH", SCREEN_WIDTH);
    shader.setInt("SCREEN_HEIGHT", SCREEN_HEIGHT);
}