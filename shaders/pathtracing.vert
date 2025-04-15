#version 460 core
layout (location = 0) in vec2 aPos;

uniform int SCREEN_WIDTH;
uniform int SCREEN_HEIGHT;

out vec2 screen_uv;

void main() {
   screen_uv = aPos * 0.5 + 0.5;
   gl_Position = vec4(aPos, 0, 1.0);
}