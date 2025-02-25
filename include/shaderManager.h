#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
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

class Shader {
private:
    // 程序ID
    GLuint ID;
    std::unordered_map<std::string, SSBO> SSBOs;
public:
    // 构造器读取并构建着色器
    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void release();
    // uniform工具函数
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;
    void setVec2(const std::string &name, const glm::vec2 &vec) const;
    template<typename T>
    void setArray(const std::string &name, const T *data, int count) const;
    void addSSBO(const std::string &name , const void *data, size_t size, GLuint binding);
    void del();
};

