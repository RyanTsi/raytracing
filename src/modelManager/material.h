#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <iostream>

using glm::vec3;

class Material {
public:
    Material(const aiMaterial* material);
    ~Material();
    std::vector<float> getData();
private:
    vec3 baseColor;
    float roughness;
    float metallic;
    float opacity;
};