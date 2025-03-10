#version 460 core

#define maxDep 16

const float PI = 3.14159265358979323846;
const float EPSILON = 0.0000001;

in vec2 screen_uv;
out vec4 FragColor;

uniform float iTime;

struct Camera {
    vec3 position;
    vec3 front;
    vec3 up;
    float fov;
    float aspectRatio;
};
uniform Camera uCamera;

float rand(vec2 co) {
	return fract((sin(dot(co + screen_uv + iTime, vec2(12.9898, 78.233)))) * 43758.5453123);
}

vec3 randomInHalfSphere(vec3 normal) {
    float phi = acos(rand(normal.zx));
    float theta = rand(normal.xy) * 2 * PI;
    vec3 u = normalize(vec3(normal.y, -normal.x, 0));
    vec3 v = normalize(cross(normal, u));
    float x = cos(theta) * cos(phi);
    float y = sin(theta) * cos(phi);
    float z = sin(phi);
    return normalize(u * x + y * v + z * normal);
}

struct Material {
    vec4 baseColor;
    float roughness;
    float metallic;
    float opacity;
};

struct Light {
    vec4 center;
    vec4 normal;
    vec4 a_vec;
    vec4 color;
    float a_len;
    float b_len;
    float power;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
};

struct Triangle {
    Vertex vert[3];
    uint materialIdx;
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
    triangle.materialIdx = data.materialIdx;
    return triangle;
};

struct Ray {
    vec3 ori;
    vec3 dir;
};

struct HitRecord {
    vec3 p;
    vec3 normal;
    float dis;
    uint materialIdx;
    bool isLight;
};

// Schlick近似Fresnel项
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX NDF
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a2 = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// 几何遮蔽函数（Smith's Schlick-GGX）
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


vec3 calBRDF(vec3 V, vec3 L, vec3 N, Material material) {
    vec3 H = normalize(V + L); // 半角向量
    // 基础颜色
    vec3 albedo = material.baseColor.rgb;
    // Fresnel项
    vec3 F0 = mix(vec3(0.04), albedo, material.metallic); // 默认为0.04，金属度为1时使用albedo
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    // 粗糙度
    float roughness = material.roughness;
    // NDF
    float D = DistributionGGX(N, H, roughness);
    // 几何遮蔽
    float G = GeometrySmith(N, V, L, roughness);
    // 镜面反射部分
    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // + 0.001防止除零
    vec3 specular = numerator / denominator;
    // 漫反射部分
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - material.metallic; // 金属没有漫反射
    vec3 diffuse = albedo / PI;
    // 最终结果
    float NdotL = max(dot(N, L), 0.0);
    return (kD * diffuse + (1 - kD) * specular) * NdotL;
}

struct RayStackEntry {
    Ray ray;
    HitRecord hitrecord;
};

Ray calFirstRay() {
    vec2 ndc_uv = screen_uv * 2 - 1.0;
    float ratio_y = tan(uCamera.fov / 2.0);
    float ratio_x = ratio_y * uCamera.aspectRatio;
    vec3 right = normalize(cross(uCamera.front, uCamera.up));
    Ray ray;
    ray.ori = uCamera.position;
    ray.dir = normalize(
        uCamera.front +
        right * ndc_uv.x * ratio_x +
        uCamera.up * ndc_uv.y * ratio_y
    );
    return ray;
}

// bool intersectTriangle(Ray ray, Triangle triangle, inout HitRecord hitrecord) {
//     vec3 a = triangle.vert[0].position, b = triangle.vert[1].position, c = triangle.vert[2].position;
//     vec3 e1 = b - a, e2 = c - a;
//     vec3 h = cross(ray.dir, e2);
//     float det = dot(e1, h);
//     if(det > -EPSILON && det < EPSILON) {
//         return false;
//     }
//     vec3 s = ray.ori - a;
//     float u = dot(s, h) / det;
//     vec3 q = cross(s, e1);
//     float v = dot(ray.dir, q) / det;
//     float w = 1 - u - v;
//     if(u < 0 || v < 0 || u + v > 1) {
//         return false;
//     }
//     float k;
//     k = dot(e2, q) / det;
//     if(k > EPSILON && k < hitrecord.dis) {
//         hitrecord.p = ray.ori + ray.dir * k;
//         hitrecord.normal = normalize(w * triangle.vert[0].normal + u * triangle.vert[1].normal + v * triangle.vert[2].normal);
//         hitrecord.dis = k;
//         hitrecord.materialIdx = triangle.materialIdx;
//         hitrecord.isLight = false;
//         return true;
//     } else {
//         return false;
//     }
// }

bool intersectTriangle(Ray ray, Triangle triangle, inout HitRecord hitrecord) {
    vec3 a = triangle.vert[0].position, b = triangle.vert[1].position, c = triangle.vert[2].position;
    vec3 e1 = b - a, e2 = c - a;
    vec3 normal = normalize(cross(e1, e2));
    vec3 h = cross(ray.dir, e2);
    float det = dot(e1, h);
    if(det > -EPSILON && det < EPSILON) {
        return false;
    }
    float k = dot(normal, a - ray.ori) / dot(normal, ray.dir);
    vec3  p = ray.ori + k * ray.dir;
    float s = dot(cross(e1, e2), normal);
    float v = dot(cross((p - a), e2), normal) / s;
    float w = dot(cross(e1, (p - a)), normal) / s;
    float u = 1 - v - w;
    if(k < EPSILON || u < 0 || w < 0 || u + w > 1 || k > hitrecord.dis) {
        return false;
    } else {
        hitrecord.p = p;
        hitrecord.normal = normalize(u * triangle.vert[0].normal + v * triangle.vert[1].normal + w * triangle.vert[2].normal);
        hitrecord.dis = k;
        hitrecord.materialIdx = triangle.materialIdx;
        hitrecord.isLight = false;
        return true;
    }
}

bool intersectLight(Ray ray, uint lightIdx, inout HitRecord hitrecord) {
    Light light = lights[lightIdx];
    vec3 u = light.a_vec.xyz, v = normalize(cross(light.a_vec.xyz, light.normal.xyz));
    float det = dot(cross(ray.dir, u), v);
    if(det > -EPSILON && det < EPSILON) {
        return false;
    }
    float r =  dot(light.normal.xyz, ray.dir);
    if(r > 0) {
        return false;
    }
    float k = dot(light.normal.xyz, light.center.xyz - ray.ori) / r;
    vec3 p = ray.ori + k * ray.dir;
    if(k > EPSILON && k < hitrecord.dis && dot(p, u) < light.a_len && dot(p, v) < light.b_len) {
        hitrecord.p = ray.ori + ray.dir * k;
        hitrecord.normal = light.normal.xyz;
        hitrecord.dis = k;
        hitrecord.materialIdx = lightIdx;
        hitrecord.isLight = true;
        return true;
    } else {
        return false;
    }
}

vec3 trace(Ray r0, HitRecord h0) {
    RayStackEntry stack[maxDep];
    int stackTop = -1;
    stack[++stackTop] = RayStackEntry(r0, h0);
    vec3 res = vec3(0);
    while(stackTop < maxDep) {
        bool hit = false;
        Ray ray_in = stack[stackTop].ray;
        HitRecord hitrecord = stack[stackTop].hitrecord;
        for(uint i = 0; i < triangles_Data.length(); i ++) {
            if(intersectTriangle(ray_in, formTriangleData(i), hitrecord)) {
                hit = true;
            }
        }
        if(stackTop != 0) {
            for(uint i = 0; i < lights.length(); i ++) {
                if(intersectLight(ray_in, i, hitrecord)) {
                    hit = true;
                }
            }
        }
        if(hit) {
            if(hitrecord.isLight == true) {
                // TODO: 
                res = lights[hitrecord.materialIdx].power * lights[hitrecord.materialIdx].color.rgb;
                while(stackTop >= 1) {
                    Ray rL = stack[stackTop--].ray;
                    Ray rV = stack[stackTop].ray;
                    Material material = materials[stack[stackTop].hitrecord.materialIdx];
                    res *= calBRDF(rV.dir, -rL.dir, stack[stackTop].hitrecord.normal, material);
                }
                break;
            } else {
                if(stackTop + 1 == maxDep) {
                    break;
                }
                Ray ray_out = Ray(hitrecord.p, randomInHalfSphere(hitrecord.normal));
                HitRecord h1 = HitRecord(vec3(0), hitrecord.normal, 1e12, 0, false);
                // res = dot(ray.dir, hitrecord.normal) * brdf * trace(ray_out, hitrecord, dep + 1);
                stack[++stackTop] = RayStackEntry(ray_out, h1);
            }
        } else {
            break;
        }
    }
    return res;
}

void main() {
    HitRecord hitrecord;
    hitrecord.dis = 1e12;
    hitrecord.normal = uCamera.front;
    Ray ray = calFirstRay();
    // float minDis;
    // bool hit = false;
    // for(uint i = 0; i < triangles_Data.length(); i ++) {
    //     Triangle triangle = formTriangleData(i);
    //     if(intersectTriangle(ray, triangle, hitrecord)) {
    //         hit = true;
    //         FragColor = vec4(0.5, 0.5, 1, 1);
    //     }
    // }
    // if(!hit) {
    //     FragColor = vec4(1, 1, 1, 1);
    // } else {
    //     FragColor = materials[hitrecord.materialIdx].baseColor;
    // }
    FragColor = vec4(trace(ray, hitrecord), 0);
}