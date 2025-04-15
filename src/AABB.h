#pragma once
#include <glm/glm.hpp>

using glm::vec3;

const float INF = 1e18;

class AABB {
    vec3 vmin(const vec3 &x, const vec3 &y);
    vec3 vmax(const vec3 &x, const vec3 &y);
public:
    vec3 mi, mx;

    AABB();
    AABB(vec3 a, vec3 b);
    void combine(const AABB &t);

    void addPoint(const vec3 &p);

    vec3 center() const;

    float surface() const;
};
