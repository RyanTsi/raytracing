#pragma once

#include <glad/gl.h>
#include <string>

class SSBO {
private:
    GLuint ssboID;
    size_t bufferSize;
    GLuint bindingPoint;
    std::string name;
public:
    SSBO(const std::string& ssboName, GLuint programID, size_t size, GLuint binding);
    SSBO();
    ~SSBO();
    void bindData(const void* data) const;
};
    