#include "SSBO.h"
#include <utility>
#include <stdexcept>

SSBO::SSBO(const std::string& ssboName, GLuint programID, size_t size, GLuint binding) 
    : name(ssboName), bufferSize(size), ssboID(0), bindingPoint(binding) {
    GLint64 limit=0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&limit);
    if (size>size_t(limit)) throw std::runtime_error("SSBO exceeds GPU limit: "+ssboName);
    glGenBuffers(1, &ssboID);
}

SSBO::SSBO() : ssboID(0), bufferSize(0), bindingPoint(0) {}

SSBO::SSBO(SSBO&& other) noexcept : SSBO() { *this = std::move(other); }
SSBO& SSBO::operator=(SSBO&& other) noexcept {
    if (this == &other) return *this;
    if (ssboID) glDeleteBuffers(1, &ssboID);
    ssboID = std::exchange(other.ssboID, 0);
    bufferSize = other.bufferSize;
    bindingPoint = other.bindingPoint;
    name = std::move(other.name);
    return *this;
}

SSBO::~SSBO() {
    if (ssboID != 0) {
        glDeleteBuffers(1, &ssboID);
        ssboID = 0;
    }
}

void SSBO::bindData(const void* data) const {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssboID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
