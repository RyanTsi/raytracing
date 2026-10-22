#include "material.h"
#include <assimp/GltfMaterial.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>
namespace {
std::string textureName(const aiMaterial* material,aiTextureType type) {
    aiString name;
    return material->GetTexture(type,0,&name)==AI_SUCCESS ? name.C_Str() : "";
}
}
Material::Material(const aiMaterial* material,TextureBank& colorBank,TextureBank& dataBank,
    const std::filesystem::path& directory,const aiScene* scene,bool backpackProfile) {
    aiColor4D color(0.8f,0.8f,0.8f,1);
    if (material->Get(AI_MATKEY_BASE_COLOR,color)!=AI_SUCCESS) material->Get(AI_MATKEY_COLOR_DIFFUSE,color);
    baseColor=glm::clamp(vec3(color.r,color.g,color.b),vec3(0),vec3(1));
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR,roughness)!=AI_SUCCESS) {
        float shininess=0;
        if (material->Get(AI_MATKEY_SHININESS,shininess)==AI_SUCCESS && shininess>0)
            roughness=std::pow(2.0f/(shininess+2.0f),0.25f);
    }
    material->Get(AI_MATKEY_METALLIC_FACTOR,metallic);
    opacity=color.a;
    material->Get(AI_MATKEY_OPACITY,opacity);
    roughness=std::clamp(roughness,0.045f,1.f); metallic=std::clamp(metallic,0.f,1.f); opacity=std::clamp(opacity,0.f,1.f);
    auto base=textureName(material,aiTextureType_BASE_COLOR);
    if (base.empty()) base=textureName(material,aiTextureType_DIFFUSE);
    auto normal=textureName(material,aiTextureType_NORMALS);
    if (normal.empty()) {
        auto height=textureName(material,aiTextureType_HEIGHT);
        // RGB normal maps are commonly exported as OBJ map_Bump. A true
        // scalar height map requires a different bump conversion.
        std::string lower=height; std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        if (lower.find("normal")!=std::string::npos) normal=height;
        else if (!height.empty()) std::cout<<"Unsupported scalar bump map (not treated as RGB normal): "<<height<<"\n";
    }
    auto rough=textureName(material,aiTextureType_DIFFUSE_ROUGHNESS);
    auto metal=textureName(material,aiTextureType_METALNESS);
    auto spec=textureName(material,aiTextureType_SPECULAR);
    auto ao=textureName(material,aiTextureType_AMBIENT_OCCLUSION);
    if (ao.empty()) ao=textureName(material,aiTextureType_LIGHTMAP);
    auto alpha=textureName(material,aiTextureType_OPACITY);
    if (backpackProfile) {
        // This bundled asset's source_attribution.txt explicitly says that
        // Joey renamed metallic to specular for a non-PBR OpenGL tutorial.
        metal=spec; spec.clear();
        if (std::filesystem::exists(directory/"roughness.jpg")) rough="roughness.jpg";
        if (std::filesystem::exists(directory/"ao.jpg")) ao="ao.jpg";
        if (!rough.empty()) roughness=1.f;
        if (!metal.empty()) metallic=1.f;
        baseColor=vec3(1);
    }
    bool packedORM=!rough.empty() && rough==metal;
    if (packedORM) { roughnessChannel=1; metallicChannel=2; }
    baseMap=colorBank.load(base,directory,scene);
    normalMap=dataBank.load(normal,directory,scene);
    roughnessMap=dataBank.load(rough,directory,scene);
    metallicMap=dataBank.load(metal,directory,scene);
    specularMap=dataBank.load(spec,directory,scene);
    occlusionMap=dataBank.load(ao,directory,scene);
    opacityMap=dataBank.load(alpha,directory,scene);
    aiString mode;
    if (material->Get(AI_MATKEY_GLTF_ALPHAMODE,mode)==AI_SUCCESS) {
        std::string value=mode.C_Str(); alphaMode=value=="MASK"?1:value=="BLEND"?2:0;
        material->Get(AI_MATKEY_GLTF_ALPHACUTOFF,alphaCutoff);
    } else if (opacity<0.999f || colorBank.hasTransparency(baseMap) || opacityMap>=0) alphaMode=2;
}
std::vector<float> Material::getData() const {
    return {baseColor.r,baseColor.g,baseColor.b,1,roughness,metallic,opacity,0};
}
std::vector<IntOrFloat> Material::getTextureData() const {
    return {baseMap,normalMap,roughnessMap,metallicMap,specularMap,occlusionMap,opacityMap,alphaMode,
        alphaCutoff,normalScale,roughnessChannel,metallicChannel};
}
