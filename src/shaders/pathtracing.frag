#version 460 core

in vec2 screen_uv;
out vec4 FragColor;

struct Material {
    float shininess;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  emissive;
    float reflact;
    float opacity;
}; 

struct Light {
    vec3 center;
    float wid;
    float len;
    vec3 norm;
    vec3 color;
    float power;
};


struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
};

struct Triangle {
    Vertex vertex[3];
    uint materialIdx;
};

layout(binding = 0, std430) readonly buffer lightBuffer {
    Light lights[];
};

layout(binding = 1, std430) readonly buffer materialBuffer {
    Material materials[];
};

layout(binding = 2, std430) readonly buffer triangleBuffer {
    Triangle triangles[];
};

struct Ray {
    vec3 ori;
    vec3 dir;
} ray;

struct Camera {
    vec3 position;
    vec3 front;
    vec3 up;
    float fov;
    float aspectRatio;
};

uniform Camera uCamera;

void calFirstRay() {
    vec2 ndc_uv = screen_uv * 2 - vec2(1.0, 1.0);
    float ratio_y = tan(uCamera.fov / 2.0);
    float ratio_x = ratio_y * uCamera.aspectRatio;
    vec3 right = normalize(cross(uCamera.front, uCamera.up));
    ray.ori = uCamera.position;
    ray.dir = normalize(
        uCamera.front +
        right * ndc_uv.x * ratio_x +
        uCamera.up * ndc_uv.y * ratio_y
    );
}

void main() {
    calFirstRay();
    // gl_FragCoord.xy;
    // FragColor = vec4(material.diffuse, 1);
    FragColor = vec4(ray.dir, 1);
    // FragColor = vec4(1, 1, 0, 1);
}