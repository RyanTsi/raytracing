#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <stb_image.h>
#include "config.h"
#include "material.h"

using glm::vec3;
using glm::vec2;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    Vertex(glm::vec3 _position, glm::vec3 _normal, glm::vec2 _texCoords)
        : position(_position), normal(_normal), texCoords(_texCoords) {}
    Vertex() {};
};

class Mesh {
private:
    // void setupMesh();
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const Material *material;
    Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, const Material *_material);
    ~Mesh();
};