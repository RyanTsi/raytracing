#include "camera.h"
#include <algorithm>
#include <cmath>

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

Camera::Camera(float x, float y, float z) {
    position        = vec3(x, y, z);
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
    move(arrow, 1.0f);
}

void Camera::move(glm::vec3 arrow, float seconds, float multiplier) {
    if (!std::isfinite(seconds) || seconds <= 0.0f ||
        !std::isfinite(multiplier) || multiplier <= 0.0f || glm::length(arrow) < 0.1f) return;
    const vec3 worldUp(0.0f, 1.0f, 0.0f);
    vec3 right = glm::cross(front, worldUp);
    if (glm::dot(right, right) < 1e-12f) right = vec3(1.0f, 0.0f, 0.0f);
    else right = glm::normalize(right);
    vec3 direction = front * arrow.z + worldUp * arrow.y + right * arrow.x;
    if (glm::dot(direction, direction) < 1e-12f) return;
    position += glm::normalize(direction) * speed * seconds * multiplier;
}

void Camera::up() {
    move(glm::vec3(0, 1, 0));
}

void Camera::down() {
    move(glm::vec3(0, -1, 0));
}

void Camera::pitch(float angle) {
    if (!std::isfinite(angle)) return;
    front = glm::normalize(front);
    float currentPitch = glm::degrees(std::asin(std::clamp(front.y, -1.0f, 1.0f)));
    float targetPitch = std::clamp(currentPitch + angle, -89.0f, 89.0f);
    vec3 horizontal(front.x, 0.0f, front.z);
    if (glm::dot(horizontal, horizontal) < 1e-12f) horizontal = vec3(0, 0, -1);
    else horizontal = glm::normalize(horizontal);
    float radians = glm::radians(targetPitch);
    front = horizontal * std::cos(radians) + vec3(0, 1, 0) * std::sin(radians);
    updateBasis();
}

void Camera::yaw(float angle) {
    if (!std::isfinite(angle)) return;
    // A fixed world-up axis prevents accumulated roll after looking up/down.
    mat4 rotateMatrix = glm::rotate(mat4(1.0f), glm::radians(std::remainder(angle, 360.0f)), vec3(0, 1, 0));
    mat3 rotateMatrix3 = mat3(rotateMatrix);
    front = rotateMatrix3 * front;
    updateBasis();
}

void Camera::updateBasis() {
    front = glm::normalize(front);
    vec3 right = glm::cross(front, vec3(0, 1, 0));
    if (glm::dot(right, right) < 1e-12f) right = vec3(1, 0, 0);
    else right = glm::normalize(right);
    upDirection = glm::normalize(glm::cross(right, front));
}

mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, upDirection);
}

mat4 Camera::getProjectionMatrix() {
    return glm::perspective(glm::radians(eyeFov), aspectRatio, zNear, zFar);
}

Camera::~Camera() {}
