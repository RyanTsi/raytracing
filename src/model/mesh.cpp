#include "mesh.h"

Mesh::Mesh(
    std::vector<Vertex> _vertices, 
    std::vector<unsigned int> _indices, 
    const Material *_material
) {
    this->vertices = _vertices;
    this->indices  = _indices;
    this->material = _material;
    // setupMesh();
}

Mesh::~Mesh() {}