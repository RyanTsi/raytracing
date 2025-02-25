#version 460 core

layout(std430, binding = 0) buffer MySsbo {
    vec4 data[];
};

void main() {
    // 使用SSBO中的数据作为顶点位置
    vec4 position = data[gl_VertexID];
    gl_Position = position;
}