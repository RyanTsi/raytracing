#include "SSBO.h"

SSBO::SSBO(const std::string& ssboName, GLuint programID, size_t size, GLuint binding) 
    : name(ssboName), bufferSize(size), ssboID(0), bindingPoint(binding) {
    glGenBuffers(1, &ssboID);
}

SSBO::SSBO() {}

SSBO::~SSBO() {
    if (ssboID != 0) {
        glDeleteBuffers(1, &ssboID);
        ssboID = 0;
    }
}

void SSBO::bindData(const void* data) const {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssboID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}