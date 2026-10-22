#include "mesh.h"
#include <utility>

Mesh::Mesh(
    std::vector<Vertex> _vertices, 
    std::vector<unsigned int> _indices, 
    size_t materialIndex_
) : vertices(std::move(_vertices)), indices(std::move(_indices)), materialIndex(materialIndex_) {}

Mesh::~Mesh() {}
