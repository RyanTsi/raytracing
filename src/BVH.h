#pragma once

#include <vector>
#include <math.h>
#include <algorithm>
#include <glm/glm.hpp>
#include "AABB.h"

using glm::vec3;

struct Triangle {
    vec3 a, b, c;
    int id;
    Triangle(vec3 _a, vec3 _b, vec3 _c, int _id) : a(_a), b(_b), c(_c), id(_id) {}
};

class BVHNode {
public:
    AABB aabb;
    BVHNode  *ls;
    BVHNode  *rs;
    Triangle *atom;
    bool isLeaf() const;
    BVHNode();
    ~BVHNode();
};

class BVH {
private:
    std::vector<Triangle> triangles;    
    std::vector<BVHNode>  nodes;
    // 计算三角形的包围盒
    AABB computeTriangleBounds(const Triangle& tri) const;

    // 递归构建 BVH
    BVHNode* buildBVH(int start, int end, int axis);

public:
    BVH();
    BVH(const std::vector<Triangle>& tris);

    // // 射线与 BVH 相交测试
    // bool intersect(const Vec3& origin, const Vec3& direction) const {
    //     return intersectRecursive(0, origin, direction);
    // }

    // bool intersectRecursive(int nodeIdx, const Vec3& origin, const Vec3& direction) const {
    //     if (nodeIdx == -1) return false;

    //     const BVHNode& node = nodes[nodeIdx];

    //     // 检查射线是否与当前节点的包围盒相交
    //     if (!rayAABBIntersect(origin, direction, node.bounds)) {
    //         return false;
    //     }

    //     // 如果是叶子节点，检查所有三角形
    //     if (node.isLeaf()) {
    //         for (int i = node.left; i < node.right; ++i) {
    //             if (rayTriangleIntersect(origin, direction, triangles[i])) {
    //                 return true;
    //             }
    //         }
    //         return false;
    //     }

    //     // 递归检查左右子树
    //     return intersectRecursive(node.left, origin, direction) ||
    //            intersectRecursive(node.right, origin, direction);
    // }

    // // 射线与 AABB 相交测试
    // bool rayAABBIntersect(const Vec3& origin, const Vec3& direction, const AABB& box) const {
    //     // 简单的 slab 方法
    //     float tmin = (box.min.x - origin.x) / direction.x;
    //     float tmax = (box.max.x - origin.x) / direction.x;
    //     if (tmin > tmax) std::swap(tmin, tmax);

    //     float tymin = (box.min.y - origin.y) / direction.y;
    //     float tymax = (box.max.y - origin.y) / direction.y;
    //     if (tymin > tymax) std::swap(tymin, tymax);

    //     if ((tmin > tymax) || (tymin > tmax)) return false;

    //     tmin = std::max(tmin, tymin);
    //     tmax = std::min(tmax, tymax);

    //     float tzmin = (box.min.z - origin.z) / direction.z;
    //     float tzmax = (box.max.z - origin.z) / direction.z;
    //     if (tzmin > tzmax) std::swap(tzmin, tzmax);

    //     if ((tmin > tzmax) || (tzmin > tmax)) return false;

    //     return true;
    // }

    // // 射线与三角形相交测试
    // bool rayTriangleIntersect(const Vec3& origin, const Vec3& direction, const Triangle& tri) const {
    //     // 使用 Möller–Trumbore 算法
    //     Vec3 edge1 = tri.v1 - tri.v0;
    //     Vec3 edge2 = tri.v2 - tri.v0;
    //     Vec3 h = direction.cross(edge2);
    //     float a = edge1.dot(h);

    //     if (a > -1e-6 && a < 1e-6) return false;

    //     float f = 1.0f / a;
    //     Vec3 s = origin - tri.v0;
    //     float u = f * s.dot(h);

    //     if (u < 0.0f || u > 1.0f) return false;

    //     Vec3 q = s.cross(edge1);
    //     float v = f * direction.dot(q);

    //     if (v < 0.0f || u + v > 1.0f) return false;

    //     float t = f * edge2.dot(q);
    //     return t > 1e-6;
    // }
};