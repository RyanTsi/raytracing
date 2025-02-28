#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <iostream>

using glm::vec4;

class alignas(16) Material {
public:
    Material(const aiMaterial* material);
    ~Material();
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float shininess;
    float reflact;
    float opacity;
private:
};

void error();