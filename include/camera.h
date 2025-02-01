#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;

class Camera {
private:
    vec3 position;
    vec3 front;
    vec3 upDirection;
    float eyeFov;
    float aspectRatio;
    float zNear;
    float zFar;
public:
    Camera();
    Camera(float x, float y, float z);
    float speed;
    float angularVelocity;
    mat4 getViewMatrix();
    mat4 getProjectionMatrix();
    void move(vec3 arrow);
    void up();
    void down();
    void pitch(float angle);
    void yaw(float angle);
    ~Camera();
};