#version 460 core

// 声明SSBO
layout(std430, binding = 0) buffer MySsboName {
    vec2 positions[];
} mySsbo;

void main() {
    // 使用SSBO中的数据作为顶点位置
    vec2 position = mySsbo.positions[gl_VertexID];
    
    gl_Position = vec4(position, 0.0, 1.0);
}