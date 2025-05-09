#include "BVH.h"

BVHNode::BVHNode() : ls(nullptr), rs(nullptr), atom(nullptr) {}

BVHNode::~BVHNode() {
    // if(ls)   delete ls;
    // if(rs)   delete rs;
    // if(atom) delete atom;
};

bool BVHNode::isLeaf() const {
    return (ls == nullptr && rs == nullptr && atom != nullptr);
}

BVH::BVH() {}

BVH::BVH(const std::vector<Triangle> &tris) : triangles(tris) {
    nodes.reserve(2 * tris.size() - 1);
    buildBVH(0, triangles.size(), 0);
}

AABB BVH::computeTriangleBounds(const Triangle& tri) const {
    vec3 minP = vec3{
        std::min({tri.a.x, tri.b.x, tri.c.x}),
        std::min({tri.a.y, tri.b.y, tri.c.y}),
        std::min({tri.a.z, tri.b.z, tri.c.z})
    };
    vec3 maxP = vec3{
        std::max({tri.a.x, tri.b.x, tri.c.x}),
        std::max({tri.a.y, tri.b.y, tri.c.y}),
        std::max({tri.a.z, tri.b.z, tri.c.z})
    };
    return AABB(minP, maxP);
};

BVHNode* BVH::buildBVH(int start, int end, int axis) {
    if (start >= end) return nullptr;

    BVHNode node;

    AABB aabb;
    for (int i = start; i < end; ++i) {
        aabb.combine(computeTriangleBounds(triangles[i]));
    }
    node.aabb = aabb;

    nodes.push_back(node);
    BVHNode *cur = &nodes.back();
    int count = end - start;
    if (count == 1) {
        cur->atom = &triangles[start];
    } else {
        // 按照指定轴排序
        std::sort(triangles.begin() + start, triangles.begin() + end,
                    [this, axis](const Triangle& a, const Triangle& b) {
                        return computeTriangleBounds(a).mi[axis] < computeTriangleBounds(b).mi[axis];
                    });
        // 分割
        int mid = start + count / 2;
        cur->ls = buildBVH(start, mid, (axis + 1) % 3);
        cur->rs = buildBVH(mid, end, (axis + 1) % 3);
    }

    // 添加到节点数组
    return cur;
}

std::vector<IntOrFloat> BVH::getData() {
    std::vector<IntOrFloat> bvhdata;
    for(const auto &i : nodes) {
        bvhdata.push_back(i.aabb.mi.x);
        bvhdata.push_back(i.aabb.mi.y);
        bvhdata.push_back(i.aabb.mi.z);
        bvhdata.push_back(0);
        bvhdata.push_back(i.aabb.mx.x);
        bvhdata.push_back(i.aabb.mx.y);
        bvhdata.push_back(i.aabb.mx.z);
        bvhdata.push_back(0);
        if(i.ls) {
            bvhdata.push_back(int(i.ls - &nodes[0]));
        } else {
            bvhdata.push_back(-1);
        }
        if(i.rs) {
            bvhdata.push_back(int(i.rs - &nodes[0]));
        } else {
            bvhdata.push_back(-1);
        }
        if(i.atom) {
            bvhdata.push_back(i.atom->id);
            bvhdata.push_back(1);
        } else {
            bvhdata.push_back(-1);
            bvhdata.push_back(0);
        }
    }
    return bvhdata;
}