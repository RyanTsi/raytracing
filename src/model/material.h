#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <vector>
#include "BVH.h"
#include "textures.h"
using glm::vec3;
class Material {
public:
    Material(const aiMaterial* material,TextureBank& colorBank,TextureBank& dataBank,
        const std::filesystem::path& directory,const aiScene* scene,bool backpackProfile=false);
    std::vector<float> getData() const;
    std::vector<IntOrFloat> getTextureData() const;
private:
    vec3 baseColor{0.8f};
    float roughness=1,metallic=0,opacity=1;
    int baseMap=-1,normalMap=-1,roughnessMap=-1,metallicMap=-1;
    int specularMap=-1,occlusionMap=-1,opacityMap=-1,alphaMode=0;
    float alphaCutoff=0.5f,normalScale=1,roughnessChannel=0,metallicChannel=0;
};
