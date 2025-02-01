#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 project;
uniform mat4 model;

void main() {
   FragPos  = vec3(model * vec4(aPos, 1.0));
   TexCoord = aTexCoord;
   Normal   = aNormal;

   // gl_Position是整个顶点着色器的输出
   gl_Position = project * view * vec4(FragPos, 1.0);
}