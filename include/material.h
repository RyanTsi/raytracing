#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>
#include <iostream>

using glm::vec3;
class Material {
public:
    Material(const aiMaterial* material);
    ~Material();
    float shininess;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float reflact;
    float opacity;
private:
};

void error();