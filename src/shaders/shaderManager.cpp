#include "shaderManager.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    // 保证ifstream对象可以抛出异常：
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        // 打开文件
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // 读取文件的缓冲内容到数据流中
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();       
        // 关闭文件处理器
        vShaderFile.close();
        fShaderFile.close();
        // 转换数据流到string
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();     
    }
    catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    // 编译着色器
    unsigned int vertexShader   = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    int success;
    char infoLog[512];

    // 顶点着色器
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);
    // 打印编译错误（如果有的话）
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

    // 片段着色器
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 着色器程序
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    // 打印连接错误（如果有的话）
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::release() {
    glUseProgram(0);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
}

void Shader::setFloat(const std::string &name, float value) const{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string &name, const glm::vec3 &vec) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setVec2(const std::string &name, const glm::vec2 &vec) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

template<typename T>
void Shader::setArray(const std::string &name, const T *data, int count) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if constexpr (std::is_same<T, float>::value) {
        glUniform1fv(location, count, data);
    } else if constexpr (std::is_same<T, glm::vec2>::value) {
        glUniform2fv(location, count, glm::value_ptr(data[0]));
    } else if constexpr (std::is_same<T, glm::vec3>::value) {
        glUniform3fv(location, count, glm::value_ptr(data[0]));
    } else if constexpr (std::is_same<T, glm::vec4>::value) {
        glUniform4fv(location, count, glm::value_ptr(data[0]));
    } else {
        // TODO: error
        return;
    }
}

void Shader::del() {
    glDeleteProgram(ID);
}

void Shader::addSSBO(const std::string &name, const void *data, size_t size, GLuint binding) {
    if(size == 0) return;
    SSBOs[name] = SSBO(name, ID, size, binding);
    SSBOs[name].bindData(data);
}

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