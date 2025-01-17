#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 rotate;

void main() {
   // gl_Position是整个顶点着色器的输出
   gl_Position = rotate * vec4(aPos.x, aPos.y, aPos.z, 1.0);
   TexCoord = aTexCoord;
   ourColor = aColor;
}