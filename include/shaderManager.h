#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
private:
public:
    // 程序ID
    unsigned int ID;
    // 构造器读取并构建着色器
    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    // uniform工具函数
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;
    template<typename T>
    void setArray(const std::string &name, const T *data, int count) const;
    void del();
};