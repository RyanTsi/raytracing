#include "material.h"

Material::Material(const aiMaterial* material) {
    aiColor3D color(0.f, 0.f, 0.f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) != AI_SUCCESS) {
        error();
    } else {
        diffuse = vec3(color.r, color.g, color.b);
    }
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) != AI_SUCCESS) {
        error();
    } else {
        specular = vec3(color.r, color.g, color.b);
    }
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) != AI_SUCCESS) {
        error();
    } else {
        ambient = vec3(color.r, color.g, color.b);
    }
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) != AI_SUCCESS) {
        error();
    } else {
        emissive = vec3(color.r, color.g, color.b);
    }
    if (material->Get(AI_MATKEY_REFRACTI, reflact) != AI_SUCCESS) {
        error();
    }
    if (material->Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS) {
        error();
    }
    if (material->Get(AI_MATKEY_SHININESS, shininess) != AI_SUCCESS) {
        error();
    }
}

Material::~Material() {}

void error() {
    std::cout << "error" << std::endl;
}