#pragma once
#include <cmath>

// Cursor callbacks own position history. A rendered frame consumes motion once;
// a frame without an event must never repeat the previous frame's movement.
class MouseLook {
public:
    struct Delta { double x=0, y=0; };
    void begin(double x,double y) {
        active=true; positioned=false; pending={};
        position(x,y);
    }
    void end() { active=false; positioned=false; pending={}; }
    void position(double x,double y) {
        if (!active) return;
        if (!std::isfinite(x) || !std::isfinite(y)) { positioned=false; return; }
        if (positioned) { pending.x+=x-lastX; pending.y+=y-lastY; }
        lastX=x; lastY=y; positioned=true;
    }
    Delta consume() { Delta result=pending; pending={}; return result; }
private:
    bool active=false, positioned=false;
    double lastX=0,lastY=0;
    Delta pending;
};
