#pragma once

#include <glm/glm.hpp>

using glm::vec3;

class Light {
public:
    Light( vec3 _center, float _a_len, float _b_len, vec3 _norm, vec3 _a_vec, vec3 _color, float _power);
    ~Light();
    std::vector<float> getData();
private:
    vec3 center;
    vec3 norm;
    vec3 a_vec;
    vec3 color;
    float a_len;
    float b_len;
    float power;
};