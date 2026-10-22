#include "BVH.h"
#include <array>
#include <limits>
#include <stdexcept>

BVH::BVH(const std::vector<Triangle>& tris) : triangles(tris) {
    if (tris.empty()) return;
    if (tris.size() > size_t(std::numeric_limits<int>::max()) / 2)
        throw std::runtime_error("Scene too large for BVH indices");
    nodes.reserve(2 * tris.size() - 1);
    buildBVH(0, int(triangles.size()), 0);
}
AABB BVH::computeTriangleBounds(const Triangle& tri) const {
    AABB box(tri.a, tri.b); box.addPoint(tri.c); return box;
}
int BVH::buildBVH(int start, int end, int depth) {
    const int index = int(nodes.size());
    nodes.emplace_back();
    AABB bounds, centroids;
    for (int i = start; i < end; ++i) {
        auto box = computeTriangleBounds(triangles[i]);
        bounds.combine(box); centroids.addPoint(box.center());
    }
    nodes[index].aabb = bounds;
    const int count = end - start;
    if (count == 1) { nodes[index].atom = triangles[start].id; return index; }
    constexpr int binCount = 12;
    float bestCost = std::numeric_limits<float>::infinity();
    int bestAxis = -1, bestSplit = -1;
    vec3 extent = centroids.mx - centroids.mi;
    auto binOf = [&](const Triangle& t, int axis) {
        float c = computeTriangleBounds(t).center()[axis];
        return std::clamp(int((c - centroids.mi[axis]) / extent[axis] * binCount), 0, binCount - 1);
    };
    // Use balanced splits after 24 SAH levels to bound GPU traversal stack use.
    if (depth < 24) for (int axis = 0; axis < 3; ++axis) {
        if (extent[axis] <= 1e-8f) continue;
        std::array<AABB, binCount> boxes;
        std::array<int, binCount> counts{};
        for (int i = start; i < end; ++i) {
            int b = binOf(triangles[i], axis);
            ++counts[b]; boxes[b].combine(computeTriangleBounds(triangles[i]));
        }
        std::array<float, binCount - 1> leftCost;
        AABB left, right;
        int nl = 0, nr = 0;
        for (int b = 0; b < binCount - 1; ++b) {
            if (counts[b]) left.combine(boxes[b]);
            nl += counts[b]; leftCost[b] = nl ? nl * left.surface() : 0;
        }
        for (int b = binCount - 1; b > 0; --b) {
            if (counts[b]) right.combine(boxes[b]);
            nr += counts[b];
            if (nr == 0 || nr == count) continue;
            float cost = leftCost[b - 1] + nr * right.surface();
            if (cost < bestCost) { bestCost = cost; bestAxis = axis; bestSplit = b - 1; }
        }
    }
    int mid = start;
    if (bestAxis >= 0) {
        auto split = std::partition(triangles.begin() + start, triangles.begin() + end,
            [&](const Triangle& t) { return binOf(t, bestAxis) <= bestSplit; });
        mid = int(split - triangles.begin());
    }
    if (mid == start || mid == end) {
        int axis = extent.y > extent.x ? 1 : 0;
        if (extent.z > extent[axis]) axis = 2;
        mid = start + count / 2;
        std::nth_element(triangles.begin() + start, triangles.begin() + mid, triangles.begin() + end,
            [&](const Triangle& a, const Triangle& b) {
                return computeTriangleBounds(a).center()[axis] < computeTriangleBounds(b).center()[axis];
            });
    }
    int ls = buildBVH(start, mid, depth + 1);
    int rs = buildBVH(mid, end, depth + 1);
    nodes[index].ls = ls; nodes[index].rs = rs;
    return index;
}
std::vector<IntOrFloat> BVH::getData() const {
    std::vector<IntOrFloat> data;
    data.reserve(nodes.size() * 12);
    for (const auto& n : nodes) {
        for (int axis = 0; axis < 3; ++axis) data.emplace_back(n.aabb.mi[axis]);
        data.emplace_back(0.f);
        for (int axis = 0; axis < 3; ++axis) data.emplace_back(n.aabb.mx[axis]);
        data.emplace_back(0.f);
        data.emplace_back(n.ls); data.emplace_back(n.rs);
        data.emplace_back(n.atom); data.emplace_back(int(n.isLeaf()));
    }
    return data;
}
