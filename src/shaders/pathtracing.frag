#version 460 core

#define maxDep 24
#define sampCnt 32

const float PI = 3.14159265358979323846; 
const float EPSILON = 0.00001;

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

vec2 seed = vec2(0);

float rand(vec2 co) {
    return fract((sin(dot(co * screen_uv + iTime + seed, vec2(82.9898, 78.233)))) * 43758.5453123);
}

vec2 rand2(vec2 p) {
    p = vec2(rand(p), rand(p * screen_uv * 100.0));
    return -1.0 + 2.0 * p;
}

vec3 randomInHalfSphere(vec3 normal, vec3 position) {
    float phi = acos(rand(normal.zx * 7.25 + position.xy * 9.17));
    float theta = rand(normal.xz * 7.97 + position.xy * 17.27) * 2 * PI;
    vec3  u = normalize(vec3(normal.y, -normal.x, 0));
    vec3  v = normalize(cross(normal, u));
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
    float a  = roughness * roughness;
    float a2 = a * a;
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
    kD *= 1.0 - material.metallic;
    vec3 diffuse = albedo / PI;
    // 最终结果
    float NdotL = max(dot(N, L), 0.0);
    return kD * diffuse + specular;
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
    if(k > EPSILON && v > 0 && w > 0 && v + w < 1 && k < hitrecord.dis) {
        hitrecord.p = p;
        hitrecord.normal = normalize(u * triangle.vert[0].normal + v * triangle.vert[1].normal + w * triangle.vert[2].normal);
        hitrecord.dis = k;
        hitrecord.materialIdx = triangle.materialIdx;
        hitrecord.isLight = false;
        return true;
    } else {
        return false;
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
    vec3 D = p - light.center.xyz;
    if(k > EPSILON && k < hitrecord.dis && 2.0 * abs(dot(D, u)) < light.a_len && 2.0 * abs(dot(D, v)) < light.b_len) {
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
        vec3 color = vec3(0);
        for(uint i = 0; i < triangles_Data.length(); i ++) {
            if(intersectTriangle(ray_in, formTriangleData(i), stack[stackTop].hitrecord)) {
                hit = true;
            }
        }
        if(stackTop != 0) {
            for(uint i = 0; i < lights.length(); i ++) {
                if(intersectLight(ray_in, i, stack[stackTop].hitrecord)) {
                    hit = true;
                }
            }
        }
        if(hit) {
            if(stack[stackTop].hitrecord.isLight == true) {
                break;
            } else {
                if(stackTop + 1 == maxDep) {
                    break;
                }
                Ray ray_out = Ray(stack[stackTop].hitrecord.p, randomInHalfSphere(stack[stackTop].hitrecord.normal, stack[stackTop].hitrecord.p));
                HitRecord h1 = HitRecord(vec3(0), vec3(0), 1e12, 0, false);
                stack[++stackTop] = RayStackEntry(ray_out, h1);
            }
        } else {
            break;
        }
    }
    Ray rL = stack[stackTop].ray, rV;
    HitRecord rec = stack[stackTop].hitrecord;
    if(stack[stackTop].hitrecord.isLight == true) {
        res = lights[rec.materialIdx].power * lights[rec.materialIdx].color.rgb * dot(lights[rec.materialIdx].normal.xyz, -rL.dir);
    }
    stackTop --;
    while(stackTop > -1) {
        rV = stack[stackTop].ray;
        rec = stack[stackTop].hitrecord;
        vec3 dirLight = vec3(0);
        for(uint i = 0; i < lights.length(); i ++) {
            vec2 luv = rand2(rec.p.yz) - 0.5;
            vec3 pT = lights[i].center.xyz + lights[i].a_vec.xyz * luv.x + normalize(cross(lights[i].normal.xyz, lights[i].a_vec.xyz)) * luv.y;
            vec3 d = pT - rec.p;
            Ray rT = Ray(rec.p, normalize(d));
            HitRecord hT; hT.dis = d.length() - 5 * EPSILON;
            bool hit2 = false;
            for(uint j = 0; j < triangles_Data.length(); j ++) {
                if(intersectTriangle(rT, formTriangleData(j), hT)) {
                    hit2 = true;
                    break;
                }
            }
            if(!hit2) {
                dirLight = lights[i].power * lights[i].color.rgb *
                           calBRDF(-rV.dir, rT.dir, rec.normal, materials[rec.materialIdx]) *
                           dot(rec.normal, rT.dir) * dot(lights[i].normal.xyz, -rT.dir) / dot(d, d) *
                           lights[i].a_len * lights[i].b_len;
            }
        }
        stackTop --;
        res =  res * calBRDF(-rV.dir, rL.dir, rec.normal, materials[rec.materialIdx]) * dot(rec.normal, rL.dir) * 2 * PI;
        // res *= dirLight;
        rL = rV;
    }

    return res;
}


void main() {
    HitRecord hitrecord;
    hitrecord.dis = 1e12;
    hitrecord.normal = uCamera.front;
    Ray ray = calFirstRay();
    vec3 resColor = vec3(0);
    for(uint i = 0; i < sampCnt; i ++) {
        resColor += trace(ray, hitrecord) / sampCnt;
        seed += rand2(screen_uv);
    }

    FragColor = vec4(resColor, 1);
}