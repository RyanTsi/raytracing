#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <iostream>

using glm::vec4;

class alignas(16) Material {
public:
    Material(const aiMaterial* material);
    ~Material();
    vec4 baseColor;
    float roughness;
    float metallic;
    float opacity;
private:
};

void error();