#version 460 core

out vec4 FragColor;

layout(std430, binding = 1) buffer MySsbo2 {
    vec4 data2[];
};

void main() {
    FragColor = vec4(0.5, 0.5, 1.0, 1.0); // 白色
}