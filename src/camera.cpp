#include "camera.h"
#include "tools.h"

Camera::Camera() {
    position        = vec3(0.0f, 0.0f,  0.0f);
    front           = vec3(0.0f, 0.0f, -1.0f);
    upDirection     = vec3(0.0f, 1.0f,  0.0f);
    eyeFov          = 45.0f;
    aspectRatio     = 1.0f * SCREEN_WIDTH / SCREEN_HEIGHT;
    zNear           = 0.1f;
    zFar            = 100.0f;
    speed           = 1.0f;
    angularVelocity = 1.0f;
}

void Camera::move(glm::vec3 arrow) {
    if(glm::length(arrow) < 0.1f) return;
    vec3 rightDirection = glm::cross(front, upDirection);
    arrow = glm::normalize(front * arrow.z + upDirection * arrow.y + rightDirection * arrow.x);
    position += arrow * speed;
}

void Camera::up() {
    move(glm::vec3(0, 1, 0));
}

void Camera::down() {
    move(glm::vec3(0, -1, 0));
}

void Camera::pitch(float angle) {
    vec3 rightDirection = glm::cross(front, upDirection);
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * PI / 180.0f, rightDirection);
    mat3 rotateMatrix3 = mat3(rotateMatrix);
    front = rotateMatrix3 * front;
    upDirection = rotateMatrix3 * upDirection;
    // norm
    front = glm::normalize(front);
    upDirection = glm::normalize(upDirection);
}

void Camera::yaw(float angle) {
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), angle * PI / 180, upDirection);
    mat3 rotateMatrix3 = mat3(rotateMatrix);
    front = rotateMatrix3 * front; 
    // norm
    front = glm::normalize(front);
}

mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, upDirection);
}

mat4 Camera::getProjectionMatrix() {
    return glm::perspective(glm::radians(eyeFov), aspectRatio, zNear, zFar);
}

Camera::~Camera() {}