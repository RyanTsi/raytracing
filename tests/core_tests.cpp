#include "BVH.h"
#include "camera.h"
#include "mouse_look.h"
#include <limits>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <set>
void require(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
bool contains(const AABB& a,const AABB& b) {
    for (int i=0;i<3;++i) if (a.mi[i]>b.mi[i]+1e-5f || a.mx[i]<b.mx[i]-1e-5f) return false;
    return true;
}
int check(const BVH& b,int i,std::set<int>& leaves,int depth=0) {
    require(i>=0 && i<int(b.nodes.size()),"Bad child index");
    require(depth<63,"GPU stack depth exceeded");
    const auto& n=b.nodes[i];
    if (n.isLeaf()) { require(leaves.insert(n.atom).second,"Duplicate primitive"); return 1; }
    require(contains(n.aabb,b.nodes.at(n.ls).aabb),"Left bound not contained");
    require(contains(n.aabb,b.nodes.at(n.rs).aabb),"Right bound not contained");
    return 1+check(b,n.ls,leaves,depth+1)+check(b,n.rs,leaves,depth+1);
}
void checkBVH(const std::vector<Triangle>& tris) {
    BVH b(tris);
    // Exercise both copy and move: the old pointer representation could dangle.
    BVH copied=b; b=BVH(); BVH moved=std::move(copied);
    if (tris.empty()) { require(moved.nodes.empty(),"Empty BVH"); return; }
    require(moved.nodes.size()==tris.size()*2-1,"Wrong node count");
    std::set<int> leaves;
    require(check(moved,0,leaves)==int(moved.nodes.size()),"Unreachable nodes");
    require(leaves.size()==tris.size(),"Missing primitives");
    for (const auto& t:tris) require(leaves.count(t.id)==1,"Lost original ID");
    auto data=moved.getData();
    require(data.size()==moved.nodes.size()*12,"Wrong std430 BVH stride");
    for (size_t i=0;i<moved.nodes.size();++i) {
        require(data[i*12+8].intValue==moved.nodes[i].ls,"Packed left child");
        require(data[i*12+10].intValue==moved.nodes[i].atom,"Packed primitive");
    }
}
int main() {
    try {
        MouseLook mouse;
        mouse.position(700,400);
        require(mouse.consume().x==0,"Uncaptured mouse moves camera");
        mouse.begin(100,100);
        require(mouse.consume().x==0,"Capture produces a delta");
        mouse.position(140,110);
        auto delta=mouse.consume();
        require(delta.x==40 && delta.y==10,"Cursor movement not accumulated");
        mouse.position(130,105);
        delta=mouse.consume();
        require(delta.x==-10 && delta.y==-5,"Reversal measured from press point");
        for (int i=0;i<4;++i) {
            delta=mouse.consume();
            require(delta.x==0 && delta.y==0,"Stationary frame repeats motion");
        }
        mouse.position(120,100); mouse.position(110,90);
        delta=mouse.consume();
        require(delta.x==-20 && delta.y==-15,"Batched cursor events lose movement");
        mouse.position(800,500);
        mouse.end();
        mouse.begin(200,200);
        delta=mouse.consume();
        require(delta.x==0 && delta.y==0,"Recapture replays pending motion");
        mouse.position(std::numeric_limits<double>::infinity(),0);
        mouse.position(500,500);
        delta=mouse.consume();
        require(delta.x==0 && delta.y==0,"Invalid coordinate poisons mouse baseline");
        mouse.position(490,480);
        delta=mouse.consume();
        require(delta.x==-10 && delta.y==-20,"Mouse cannot recover after invalid position");
        checkBVH({});
        checkBVH({Triangle(vec3(0),vec3(1,0,0),vec3(0,1,0),0)});
        std::vector<Triangle> random,coincident;
        std::mt19937 gen(42); std::uniform_real_distribution<float> d(-100,100);
        for (int i=0;i<4096;++i) {
            vec3 p(d(gen),d(gen),d(gen));
            random.emplace_back(p,p+vec3(0.1f,0,0),p+vec3(0,0.1f,0),i);
            coincident.emplace_back(vec3(0),vec3(1,0,0),vec3(0,1,0),i);
        }
        checkBVH(random); checkBVH(coincident);
        AABB a(vec3(-1),vec3(1));
        require(std::abs(a.surface()-24.f)<1e-6f,"AABB surface");
        Camera c(0,0,12);
        require(std::abs(c.getProjectionMatrix()[1][1]-1.f/std::tan(glm::radians(45.f)/2))<1e-5f,"Camera FOV");
        for (int i=0;i<1000;++i) { c.yaw(0.5f); c.pitch(0.1f); }
        require(std::abs(glm::length(c.front)-1)<1e-5,"Camera normalization");
        require(std::abs(glm::dot(c.front,c.upDirection))<1e-4,"Camera basis");
        require(std::abs(glm::cross(c.front,c.upDirection).y)<1e-5f,"Camera accumulated roll");
        Camera pole;
        pole.pitch(10000.f);
        require(std::abs(glm::degrees(std::asin(pole.front.y))-89.f)<0.001f,"Pitch upper limit");
        pole.yaw(90.f);
        require(std::abs(glm::degrees(std::asin(pole.front.y))-89.f)<0.001f,"Yaw changes pitch near pole");
        pole.pitch(-10.f);
        require(std::abs(glm::degrees(std::asin(pole.front.y))-79.f)<0.001f,"Pitch cannot recover from upper limit");
        pole.pitch(-10000.f);
        require(std::abs(glm::degrees(std::asin(pole.front.y))+89.f)<0.001f,"Pitch lower limit");
        require(pole.upDirection.y>0.f,"Camera flips upside down");
        Camera horizontal;
        horizontal.pitch(30.f); horizontal.yaw(90.f);
        require(glm::length(horizontal.front-vec3(-std::sqrt(0.75f),0.5f,0.f))<1e-5f,"World-up yaw");
        Camera movement30(0,0,12), movement144=movement30;
        movement30.speed=movement144.speed=5.f;
        for (int i=0;i<30;++i) movement30.move(vec3(1,0,1),1.f/30,0.5f);
        for (int i=0;i<144;++i) movement144.move(vec3(1,0,1),1.f/144,0.5f);
        require(glm::length(movement30.position-movement144.position)<1e-4f,"Movement depends on frame rate");
        require(std::abs(glm::length(movement30.position-vec3(0,0,12))-2.5f)<1e-4f,"Diagonal movement accelerates");
        Camera vertical;
        vertical.pitch(75.f);
        vertical.move(vec3(0,1,0),0.5f);
        require(glm::length(vertical.position-vec3(0,0.5f,0))<1e-6f,"Q/E movement is not world vertical");
        std::cout<<"PASS: empty/single/random/coincident BVH, copy/move, packed layout, camera\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
