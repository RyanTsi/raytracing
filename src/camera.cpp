#include "camera.h"
#include "tools.h"

camera::camera() {
    position    = vec3(0.0f, 0.0f, 0.0f);
    Front       = vec3(0.0f, 0.0f, -1.0f);
    upDirection = vec3(0.0f, 1.0f, 0.0f);
}

void camera::move(float dx, float dy, float dz) {
    position.x += dx;
    position.y += dy;
    position.z += dz;
}

void camera::pitch(float angle) {
    vec3 rightDirection = glm::cross(Front, upDirection);
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * 2.0f * PI / 180.0f, rightDirection);
    vec4 _Front = vec4(Front, 0.0f);
    _Front = rotateMatrix * _Front;
    Front = vec3(_Front.x, _Front.y, _Front.z);
    vec4 _upDirection = vec4(upDirection, 0.0f);
    _upDirection = rotateMatrix * _upDirection;
    upDirection = vec3(_upDirection.x, _upDirection.y, _upDirection.z);
}

void camera::yaw(float angle) {
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * 2 * PI / 180, upDirection);
    vec4 _Front = vec4(Front, 0.0f);
    _Front = rotateMatrix * _Front;
    Front = vec3(_Front.x, _Front.y, _Front.z);
}

mat4 camera::getViewMatrix() {
    return glm::lookAt(position, position + Front, upDirection);
}

camera::~camera() {}