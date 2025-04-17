#version 460 core
layout (location = 0) in vec2 aPos;

uniform int SCREEN_WIDTH;
uniform int SCREEN_HEIGHT;
uniform mat4 jitterMatrix;

out vec2 screen_uv;

void main() {
   screen_uv = aPos * 0.5 + 0.5;
   gl_Position = jitterMatrix * vec4(aPos, 0.0, 1.0); // 应用抖动
}