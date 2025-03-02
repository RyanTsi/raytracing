#version 460 core

const float PI = 3.14159265358979323846;
const float EPSILON = 0.0000001;

in vec2 screen_uv;
out vec4 FragColor;

struct Camera {
    vec3 position;
    vec3 front;
    vec3 up;
    float fov;
    float aspectRatio;
};
uniform Camera uCamera;

float pow2(float a) {
    return a * a;
}


uint wseed = 3127143136u;
uint whash(uint seed) {
    seed = (seed ^ uint(61)) ^ (seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> uint(4));
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> uint(15));
    return seed;
}

float randcore4() {
	wseed = whash(wseed);
	return fract(sin(dot(screen_uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

float rand() {
	return randcore4();
}

vec3 randomInHalfSphere(vec3 normal) {
    float phi = acos(rand());
    float theta = rand() * 2 * PI;
    vec3 u = normalize(vec3(normal.y, -normal.x, 0));
    vec3 v = normalize(cross(normal, u));
    float x = cos(theta) * cos(phi);
    float y = sin(theta) * cos(phi);
    float z = sin(phi);
    return normalize(u * x + y * v + z * normal);
}

struct Material {
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec4  emissive;
    float shininess;
    float reflact;
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


// Fresnel Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX Normal Distribution Function
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = pow2(roughness);
    float a2 = pow2(a);
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = pow2(NdotH);
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * pow2(denom);

    return nom / denom;
}

// Geometry Schlick GGX
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = pow2(r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// Geometry Smith
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 calBRDF(vec3 lightDir, vec3 viewDir, vec3 normal, Material material) {
    vec3 halfVec = normalize(lightDir + viewDir);
    float roughness = sqrt(2.0 / (material.shininess + 2.0));
    vec3  F = fresnelSchlick(max(dot(halfVec, viewDir), 0), material.specular.xyz);
    float D = distributionGGX(normal, halfVec, roughness);
    float G = geometrySmith(normal, viewDir, lightDir, roughness);
    // Specular BRDF
    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);
    
    // Diffuse BRDF
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = (kD * material.diffuse.xyz) / PI;
    
    // Combine diffuse and specular components
    return (diffuse + specular) * NdotL;
}

struct RayStackEntry {
    Ray ray;
    HitRecord hitrecord;
};

const uint maxDep = 10;

Ray calFirstRay() {
    vec2 ndc_uv = screen_uv * 2 - vec2(1.0, 1.0);
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
    vec3 h = cross(ray.dir, e2);
    float det = dot(e1, h);
    if(det > -EPSILON && det < EPSILON) {
        return false;
    }
    vec3 s = ray.ori - a;
    float u = dot(s, h) / det;
    vec3 q = cross(s, e1);
    float v = dot(ray.dir, q) / det;
    float w = 1 - u - v;
    if(u < 0 || v < 0 || u + v > 1) {
        return false;
    }
    float k;
    k = dot(e2, q) / det;
    if(k > EPSILON && k < hitrecord.dis) {
        hitrecord.p = ray.ori + ray.dir * k;
        hitrecord.normal = normalize(w * triangle.vert[0].normal + u * triangle.vert[1].normal + v * triangle.vert[2].normal);
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
    float k = dot(light.normal.xyz, light.center.xyz - ray.ori) / dot(light.normal.xyz, ray.dir);
    vec3 T = ray.ori + k * ray.dir - light.center.xyz;
    if(k > EPSILON && k < hitrecord.dis && dot(T, u) < light.a_len && dot(T, v) < light.b_len) {
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
                res = lights[hitrecord.materialIdx].power * lights[hitrecord.materialIdx].color.rgb;
                while(stackTop >= 1) {
                    Ray r0 = stack[stackTop].ray;
                    stackTop --;
                    Ray r1 = stack[stackTop].ray;
                    Material material = materials[stack[stackTop].hitrecord.materialIdx];
                    res *= material.diffuse.xyz;
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
    //     FragColor = materials[hitrecord.materialIdx].diffuse;
    // }
    FragColor = vec4(trace(ray, hitrecord), 0);


    // bool hit = false;
    // for(int i = 0; i < lights.length(); i ++) {
    //     if(intersectLight(ray, i, hitrecord)) {
    //         hit = true;
    //     }
    // }
    // if(hit) {
    //         FragColor = vec4(1, 1, 1, 1);
    // } else {
    //         FragColor = vec4(0, 0, 0, 1);
    // }
}