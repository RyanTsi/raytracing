#version 460 core

in vec2 screen_uv;
out vec4 FragColor;

struct Material {
    float shininess;
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec4  emissive;
    float reflact;
    float opacity;
}; 

struct Light {
    vec4 center;
    vec4 w_l_p_t; // width-length-power-type
    vec4 norm;
    vec4 color;
};


struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
};

struct Triangle {
    Vertex vert[3];
};


struct TriangleData {
    float position[9];
    float normal[9];
    float uv[6];
    uint materialIdx;
};

layout(std430, binding = 1) buffer lightBuffer {
    Light lights[];
};

layout(std430, binding = 2) buffer materialBuffer {
    Material materials[];
};

layout(std430, binding = 3) buffer triangleBuffer {
    TriangleData triangles_Data[];
};

Triangle formTriangleData(uint idx) {
    TriangleData data = triangles_Data[idx];
    Triangle triangle;
    for(uint i = 0; i < 3; i ++) {
        triangle.vert[i].position = vec3(data.position[i * 3 + 0], data.position[i * 3 + 1], data.position[i * 3 + 2]);
        triangle.vert[i].normal = vec3(data.normal[i * 3 + 0], data.normal[i * 3 + 1], data.normal[i * 3 + 2]);
        if(i < 2) {
            triangle.vert[i].texCoords = vec2(data.uv[i * 3 + 0], data.uv[i * 3 + 1]);
        }
    }
    return triangle;
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

bool intersectTriangle(Ray ray, vec3 a, vec3 b, vec3 c, out float k) {
    const float EPSILON = 0.0000001;
    vec3 e1 = b - a, e2 = c - a;
    vec3 h = cross(ray.dir, e2);
    float det = dot(e1, h);
    if(det > -EPSILON && det < EPSILON) {
        return false;
    }
    vec3 s = ray.ori - a;
    float u = dot(s, h) / det;
    vec3 q = cross(s, e1);
    float v = dot(ray.dir, q) / det;
    if(u < 0 || v < 0 || u + v > 1) {
        return false;
    }
    k = dot(e2, q) / det;
    if(k > EPSILON) {
        return true;
    } else {
        return false;
    }
}

void main() {
    calFirstRay();
    float k;
    for(uint i = 0; i < triangles_Data.length(); i ++) {
        Triangle triangle = formTriangleData(i);
        if(intersectTriangle(ray, triangle.vert[0].position, triangle.vert[1].position, triangle.vert[2].position, k)) {
            FragColor = vec4(0.5, 0.5, 1, 1);
        } else {
            FragColor = vec4(1, 1, 1, 1);
        }
    }
}