#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat4;

class camera {
private:
    vec3 position;
    vec3 Front;
    vec3 upDirection;
public:
    camera();
    mat4 getViewMatrix();
    void move(float dx, float dy, float dz);
    void pitch(float angle);
    void yaw(float angle);
    ~camera();
};