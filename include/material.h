#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <iostream>

using glm::vec4;

class Material {
public:
    Material(const aiMaterial* material);
    ~Material();
    float shininess;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float reflact;
    float opacity;
private:
};

void error();