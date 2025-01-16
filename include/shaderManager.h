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
    // 程序ID
    unsigned int ID;
public:
    // 构造器读取并构建着色器
    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    // uniform工具函数
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, glm::mat4 mat) const;
    void del();
};