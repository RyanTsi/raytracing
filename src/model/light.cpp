#include "light.h"

Light::Light(
    vec3 _center, 
    float _a_len, 
    float _b_len, 
    vec3 _norm, 
    vec3 _a_vec, 
    vec3 _color, 
    float _power
) : center(_center), a_len(_a_len), b_len(_b_len), power(_power) {
    a_vec  = glm::normalize(_a_vec);
    norm   = glm::normalize(_norm);
    color  = glm::normalize(_color);
}

Light::~Light() {}

std::vector<float> Light::getData() {
    std::vector<float> res;
    res.push_back(center.x);
    res.push_back(center.y);
    res.push_back(center.z);
    res.push_back(1);
    res.push_back(norm.x);
    res.push_back(norm.y);
    res.push_back(norm.z);
    res.push_back(1);
    res.push_back(a_vec.x);
    res.push_back(a_vec.y);
    res.push_back(a_vec.z);
    res.push_back(0);
    res.push_back(color.r);
    res.push_back(color.g);
    res.push_back(color.b);
    res.push_back(1);
    res.push_back(a_len);
    res.push_back(b_len);
    res.push_back(power);
    res.push_back(power);
    return res;
}