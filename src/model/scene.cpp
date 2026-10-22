#include "scene.h"
#include <stdexcept>
#include <filesystem>
#include <utility>

Scene::Scene(const char *path,const std::vector<std::string>& attachments) : attachmentCount_(attachments.size()) {
    isUpdate = true;
    loadScene(path);
    for (const auto& attachment:attachments) loadScene(attachment);
    colorTextures.upload();
    dataTextures.upload();
    setFrame();
    AABB all, focus;
    bool hasFocus = false;
    for (const auto& mesh : meshes) {
        AABB box;
        for (const auto& v : mesh.vertices) box.addPoint(v.position);
        all.combine(box);
        vec3 extent = box.mx-box.mi;
        float largest = std::max({extent.x,extent.y,extent.z});
        // Ignore planar floors/walls even when rotated away from the axes.
        bool solid = false;
        for (size_t i=0;i+2<mesh.indices.size();i+=3) {
            vec3 a=mesh.vertices[mesh.indices[i]].position;
            vec3 n=glm::cross(mesh.vertices[mesh.indices[i+1]].position-a,
                             mesh.vertices[mesh.indices[i+2]].position-a);
            if (glm::length(n)<largest*largest*1e-8f) continue;
            n=glm::normalize(n);
            for (const auto& v:mesh.vertices)
                if (std::abs(glm::dot(v.position-a,n))>largest*1e-5f) { solid=true; break; }
            break;
        }
        if (solid) {
            focus.combine(box); hasFocus = true;
        }
    }
    if (!hasFocus) focus = all;
    vec3 center = focus.center();
    float radius = std::max(glm::length(focus.mx-focus.mi)*0.5f, 0.01f);
    camera = Camera(center.x, center.y+radius*0.35f, center.z+radius*2.9f);
    camera.front = glm::normalize(center-camera.position);
    camera.upDirection = glm::normalize(glm::cross(glm::cross(camera.front,vec3(0,1,0)),camera.front));
    camera.speed = radius;
    homeCamera = camera;
    sceneEpsilon = std::max(radius*1e-5f, 1e-7f);
    vec3 lightCenter = center+vec3(-radius*0.8f,radius*1.8f,radius*0.8f);
    addLight(Light(lightCenter,radius*1.2f,radius*1.2f,
        glm::normalize(center-lightCenter),vec3(1,0,0),vec3(1.0f,0.95f,0.85f),12.0f));
}

Scene::~Scene() {
    glDeleteBuffers(1,&VBO);
    glDeleteVertexArrays(1,&VAO);
}

void Scene::loadScene(const std::string &path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_GenSmoothNormals      |
        aiProcess_PreTransformVertices);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)  {
        throw std::runtime_error(std::string("ASSIMP: ")+importer.GetErrorString());
    }
    auto modelPath=std::filesystem::u8path(path);
    colorTextures.beginModel();
    dataTextures.beginModel();
    directory = modelPath.parent_path().u8string();
    materialOffset=materials.size();
    materials.reserve(materialOffset+scene->mNumMaterials);
    const bool backpackProfile=modelPath.filename()=="backpack.obj" &&
        std::filesystem::exists(modelPath.parent_path()/"source_attribution.txt");
    for(unsigned int i = 0; i < scene->mNumMaterials; i ++) {
        const aiMaterial *material = scene->mMaterials[i];
        materials.emplace_back(material,colorTextures,dataTextures,modelPath.parent_path(),scene,backpackProfile);
    }
    std::cout<<"Imported "<<scene->mNumMeshes<<" meshes, "<<materials.size()<<" materials, "
        <<colorTextures.count()<<" color textures, "<<dataTextures.count()<<" data textures\n";

    processNode(scene->mRootNode, scene);
    if (meshes.empty()) throw std::runtime_error("Scene has no triangle meshes");
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
    vertices.reserve(mesh->mNumVertices);
    indices.reserve(size_t(mesh->mNumFaces)*3);
    for(unsigned int i = 0; i < mesh->mNumVertices; i ++) {
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
    for(unsigned int i = 0; i < mesh->mNumFaces; i ++) {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices != 3) continue;
        for(unsigned int j = 0; j < face.mNumIndices; j ++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    return Mesh(std::move(vertices),std::move(indices),materialOffset+mesh->mMaterialIndex);
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
    textureMaterialsBuffer.clear();
    std::vector<Triangle> trs;
    int triangleCnt = 0;
    for(const auto& mesh : meshes) {
        for(unsigned int i = 0; i < mesh.indices.size(); i += 3) {
            trianglesBuffer.push_back(
                TriangleData(mesh.vertices[mesh.indices[i]],
                    mesh.vertices[mesh.indices[i + 1]],
                    mesh.vertices[mesh.indices[i + 2]],
                    static_cast<GLuint>(mesh.materialIndex)
                ));
            trs.push_back(
                Triangle(mesh.vertices[mesh.indices[i]].position,
                    mesh.vertices[mesh.indices[i + 1]].position,
                    mesh.vertices[mesh.indices[i + 2]].position,
                    triangleCnt
                ));
            triangleCnt ++;
        }
    }
    bvh = BVH(trs);
    if (trs.empty()) throw std::runtime_error("Scene has no triangles");
    for(auto light : lights) {
        std::vector<float> temp = light.getData();
        lightsBuffer.insert(lightsBuffer.end(), temp.begin(), temp.end());
    }
    for(const auto& material : materials) {
        std::vector<float> temp = material.getData();
        materialsBuffer.insert(materialsBuffer.end(), temp.begin(), temp.end());
        auto textureData=material.getTextureData();
        textureMaterialsBuffer.insert(textureMaterialsBuffer.end(),textureData.begin(),textureData.end());
    }
    isUpdate = false;
}

void Scene::addLight(Light light) {
    lights.push_back(light);
    isUpdate = true;
}

void Scene::prepare(Shader &shader) {
    if(isUpdate) {
        updateBuffer();
        setBuffer(shader);
    }
    colorTextures.bind(1);
    dataTextures.bind(2);
    shader.setInt("colorTextures",1);
    shader.setInt("dataTextures",2);
    glActiveTexture(GL_TEXTURE0);
}

void Scene::draw(Shader &shader) {
    prepare(shader);
    setCamera(shader);
    drawQuad();
}

void Scene::drawQuad() const {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES,0,6);
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
    shader.addSSBO("textureMaterialBuffer",textureMaterialsBuffer.data(),textureMaterialsBuffer.size()*sizeof(IntOrFloat),6);
    shader.addSSBO("triangleBuffer", trianglesBuffer.data(), trianglesBuffer.size() * sizeof(TriangleData), 3);
    std::vector<IntOrFloat> bvhdata = bvh.getData();
    shader.addSSBO("BVH", bvhdata.data(), bvhdata.size() * sizeof(IntOrFloat), 4);
}
