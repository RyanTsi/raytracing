#include "camera.h"
#include "tools.h"

Camera::Camera() {
    position    = vec3(0.0f, 0.0f, 0.0f);
    Front       = vec3(0.0f, 0.0f, -1.0f);
    upDirection = vec3(0.0f, 1.0f, 0.0f);
}

void Camera::move(float dx, float dy, float dz) {
    position.x += dx;
    position.y += dy;
    position.z += dz;
}

void Camera::pitch(float angle) {
    vec3 rightDirection = glm::cross(Front, upDirection);
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * PI / 180.0f, rightDirection);
    mat3 rotateMatrix3 = mat3(rotateMatrix);
    Front = rotateMatrix3 * Front;
    upDirection = rotateMatrix3 * upDirection;
    // norm
    Front = glm::normalize(Front);
    upDirection = glm::normalize(upDirection);
}

void Camera::yaw(float angle) {
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * 2 * PI / 180, upDirection);
    mat3 rotateMatrix3 = mat3(rotateMatrix);
    Front = rotateMatrix3 * Front; 
    // norm
    Front = glm::normalize(Front);
}

mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + Front, upDirection);
}

Camera::~Camera() {}