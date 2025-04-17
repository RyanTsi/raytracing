#include "AABB.h"

vec3 AABB::vmin(const vec3 &x, const vec3 &y) {
    return {std::min(x[0], y[0]), std::min(x[1], y[1]), std::min(x[2], y[2])};
}

vec3 AABB::vmax(const vec3 &x, const vec3 &y) {
    return {std::max(x[0], y[0]), std::max(x[1], y[1]), std::max(x[2], y[2])};
}

AABB::AABB(): mi(INF), mx(-INF) {}

AABB::AABB(vec3 a, vec3 b) {
    mi = vmin(a, b);
    mx = vmax(a, b);
}

void AABB::combine(const AABB &t) {
    mi = vmin(mi, t.mi);
    mx = vmax(mx, t.mx);
}

void AABB::addPoint(const vec3 &p) {
    mi = vmin(mi, p);
    mx = vmax(mx, p);
}

vec3 AABB::center() const {
    return (mi + mx) * 0.5f;
}

float AABB::surface() const {
    vec3 t = mx - mi;
    return 2.0f * (t.x * t.y + t.y * t.z + t.z * t.x);
}