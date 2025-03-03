#include "material.h"

Material::Material(const aiMaterial* material) {
    aiColor3D color(0.f, 0.f, 0.f);
    if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
        baseColor = vec4(color.r, color.g, color.b, 1.0f);
    } else if(material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        baseColor = vec4(color.r, color.g, color.b, 1.0f);
    } else {
        baseColor = vec4(0.8, 0.8, 0.8, 1.0f);
    }
    // if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
    //     specular  = vec4(color.r, color.g, color.b, 1.0f);
    // } else {
    //     specular  = vec4(0.04, 0.04, 0.04, 1.0f);
    // }
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != AI_SUCCESS) {
        roughness = 1.0f;
    }
    if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != AI_SUCCESS) {
        metallic = 1.0f;
    }
    if (material->Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS) {
        opacity  = 1.0f;
    }
}

Material::~Material() {}

void error() {
    std::cout << "error" << std::endl;
}