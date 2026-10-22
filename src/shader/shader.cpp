#include "shader.h"
#include <stdexcept>
#include <vector>
namespace {
GLuint compile(GLenum type, const std::string& code) {
    GLuint s=glCreateShader(type);
    const char* text=code.c_str();
    glShaderSource(s,1,&text,nullptr); glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if (!ok) {
        GLint length; glGetShaderiv(s,GL_INFO_LOG_LENGTH,&length);
        std::vector<char> log(size_t(length)+1);
        glGetShaderInfoLog(s,length,nullptr,log.data()); glDeleteShader(s);
        throw std::runtime_error(std::string("Shader compilation: ")+log.data());
    }
    return s;
}
}
Shader::Shader(const char* vertexPath,const char* fragmentPath) : ID(0) {
    const auto vertexCode=read_file(vertexPath), fragmentCode=read_file(fragmentPath);
    GLuint v=compile(GL_VERTEX_SHADER,vertexCode), f=0;
    try { f=compile(GL_FRAGMENT_SHADER,fragmentCode); }
    catch (...) { glDeleteShader(v); throw; }
    ID=glCreateProgram(); glAttachShader(ID,v); glAttachShader(ID,f); glLinkProgram(ID);
    glDeleteShader(v); glDeleteShader(f);
    GLint ok=0; glGetProgramiv(ID,GL_LINK_STATUS,&ok);
    if (!ok) {
        GLint length; glGetProgramiv(ID,GL_INFO_LOG_LENGTH,&length);
        std::vector<char> log(size_t(length)+1);
        glGetProgramInfoLog(ID,length,nullptr,log.data()); glDeleteProgram(ID); ID=0;
        throw std::runtime_error(std::string("Shader linking: ")+log.data());
    }
}
Shader::~Shader() { del(); }
GLint Shader::location(const std::string& name) const {
    auto it=locations.find(name);
    if (it!=locations.end()) return it->second;
    return locations.emplace(name,glGetUniformLocation(ID,name.c_str())).first->second;
}
void Shader::use() { glUseProgram(ID); }
void Shader::release() { glUseProgram(0); }
void Shader::setBool(const std::string& n,bool v) const { glUniform1i(location(n),v); }
void Shader::setInt(const std::string& n,int v) const { glUniform1i(location(n),v); }
void Shader::setFloat(const std::string& n,float v) const { glUniform1f(location(n),v); }
void Shader::setMat4(const std::string& n,const glm::mat4& v) const { glUniformMatrix4fv(location(n),1,GL_FALSE,glm::value_ptr(v)); }
void Shader::setVec3(const std::string& n,const glm::vec3& v) const { glUniform3fv(location(n),1,glm::value_ptr(v)); }
void Shader::setVec2(const std::string& n,const glm::vec2& v) const { glUniform2fv(location(n),1,glm::value_ptr(v)); }
void Shader::del() { if (ID) { glDeleteProgram(ID); ID=0; } }
void Shader::addSSBO(const std::string& name,const void* data,size_t size,GLuint binding) {
    if (!size) throw std::runtime_error("Empty SSBO: "+name);
    SSBOs.insert_or_assign(name,SSBO(name,ID,size,binding));
    SSBOs.at(name).bindData(data);
}
