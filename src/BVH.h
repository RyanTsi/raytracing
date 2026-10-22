#pragma once
#include <algorithm>
#include <vector>
#include "AABB.h"
union IntOrFloat {
    int intValue; float floatValue;
    IntOrFloat(int value) : intValue(value) {}
    IntOrFloat(float value) : floatValue(value) {}
};
static_assert(sizeof(IntOrFloat) == 4);
struct Triangle {
    vec3 a, b, c;
    int id;
    Triangle(vec3 a_, vec3 b_, vec3 c_, int id_) : a(a_), b(b_), c(c_), id(id_) {}
};
// Indices survive copies, moves and vector reallocations.
struct BVHNode {
    AABB aabb;
    int ls = -1, rs = -1, atom = -1;
    bool isLeaf() const { return atom >= 0; }
};
class BVH {
    std::vector<Triangle> triangles;
    AABB computeTriangleBounds(const Triangle& tri) const;
    int buildBVH(int start, int end, int depth);
public:
    std::vector<BVHNode> nodes;
    BVH() = default;
    explicit BVH(const std::vector<Triangle>& tris);
    std::vector<IntOrFloat> getData() const;
};
