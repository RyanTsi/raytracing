#version 460 core
layout (location = 0) in vec2 aPos;

uniform int SCREEN_WIDTH;
uniform int SCREEN_HEIGHT;

out vec2 screen_uv;

void main() {
   screen_uv = (aPos + 1.0) / 2;
   // gl_Position 是整个顶点着色器的输出
   gl_Position = vec4(aPos, 0, 1.0);
}