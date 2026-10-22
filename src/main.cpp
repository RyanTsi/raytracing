#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "model/scene.h"
#include "mouse_look.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fs = std::filesystem;
struct Options {
    fs::path root = fs::u8path(RTRT_SOURCE_DIR), scene, output;
    std::vector<fs::path> attachments;
    int width=1280, height=800, frames=0, bounces=6, seed=1, debugView=0;
    float scale=1.f, exposure=0.f, environment=0.03f;
    float lookSensitivity=0.04f, moveSpeed=0.5f;
    bool headless=false, nee=true, mis=true, bvh=true, autoTraversal=true, uniform=false, selfTest=false;
};
std::vector<std::string> commandLine(int argc,char** argv) {
#ifdef _WIN32
    // The narrow CRT argv uses the active ANSI code page. PMX paths frequently
    // contain Chinese/Japanese characters, so keep the full Unicode command line.
    int count=0;
    wchar_t** wide=CommandLineToArgvW(GetCommandLineW(),&count);
    if (!wide) throw std::runtime_error("Cannot decode Windows command line");
    std::vector<std::string> args;
    try {
        args.reserve(count);
        for (int i=0;i<count;++i) {
            int length=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide[i],-1,nullptr,0,nullptr,nullptr);
            if (!length) throw std::runtime_error("Invalid Unicode command line");
            std::string value(size_t(length),'\0');
            if (!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide[i],-1,value.data(),length,nullptr,nullptr))
                throw std::runtime_error("Cannot convert Unicode command line");
            value.pop_back();
            args.push_back(std::move(value));
        }
    } catch (...) { LocalFree(wide); throw; }
    LocalFree(wide);
    return args;
#else
    return std::vector<std::string>(argv,argv+argc);
#endif
}
Options parse(const std::vector<std::string>& args) {
    Options o;
    for (size_t i=1;i<args.size();++i) {
        std::string arg=args[i];
        auto value=[&]() -> std::string {
            if (++i>=args.size()) throw std::runtime_error("Missing value for "+arg);
            return args[i];
        };
        if (arg=="--headless") o.headless=true;
        else if (arg=="--scene") o.scene=fs::u8path(value());
        else if (arg=="--attachment") o.attachments.push_back(fs::u8path(value()));
        else if (arg=="--output") o.output=fs::u8path(value());
        else if (arg=="--root") o.root=fs::u8path(value());
        else if (arg=="--width") o.width=std::stoi(value());
        else if (arg=="--height") o.height=std::stoi(value());
        else if (arg=="--frames") o.frames=std::stoi(value());
        else if (arg=="--bounces") o.bounces=std::stoi(value());
        else if (arg=="--seed") o.seed=std::stoi(value());
        else if (arg=="--scale") o.scale=std::stof(value());
        else if (arg=="--exposure") o.exposure=std::stof(value());
        else if (arg=="--environment") o.environment=std::stof(value());
        else if (arg=="--look-sensitivity") o.lookSensitivity=std::stof(value());
        else if (arg=="--move-speed") o.moveSpeed=std::stof(value());
        else if (arg=="--view") {
            std::string view=value();
            if (view=="path") o.debugView=0;
            else if (view=="albedo") o.debugView=1;
            else if (view=="normal") o.debugView=2;
            else throw std::runtime_error("View must be path, albedo or normal");
        }
        else if (arg=="--no-nee") o.nee=false;
        else if (arg=="--no-mis") o.mis=false;
        else if (arg=="--brute-force") { o.bvh=false; o.autoTraversal=false; }
        else if (arg=="--force-bvh") { o.bvh=true; o.autoTraversal=false; }
        else if (arg=="--uniform-sampling") o.uniform=true;
        else if (arg=="--self-test") { o.selfTest=true; o.headless=true; o.frames=12; }
        else if (arg=="--help") {
            std::cout<<"rtrt [--scene file.obj|file.gltf|file.glb|file.pmx] [--root resource-directory]\n"
                <<"  --attachment file (repeatable; merge in original model coordinates)\n"
                <<"  --headless --frames N --width W --height H --output image.ppm\n"
                <<"  --scale 0.25..1 --bounces 1..16 --seed N --exposure EV\n"
                <<"  --environment radiance --no-nee --no-mis --uniform-sampling --brute-force --force-bvh --self-test\n"
                <<"  --look-sensitivity 0.005..0.2 (degrees per mouse unit, default 0.04)\n"
                <<"  --move-speed 0.05..3 (scene-relative movement multiplier, default 0.5)\n"
                <<"  --view path|albedo|normal (default path)\n"
                <<"PPM output also writes linear HDR .pfm and measurements .json.\n";
            std::exit(0);
        } else throw std::runtime_error("Unknown option: "+arg);
    }
    if (o.width<32 || o.height<32 || o.width>8192 || o.height>8192 ||
        o.frames<0 || o.frames>1000000 || o.bounces<1 || o.bounces>16 ||
        !std::isfinite(o.scale) || o.scale<0.25f || o.scale>1.f ||
        !std::isfinite(o.exposure) || std::abs(o.exposure)>20.f ||
        !std::isfinite(o.environment) || o.environment<0.f || o.environment>100.f ||
        !std::isfinite(o.lookSensitivity) || o.lookSensitivity<0.005f || o.lookSensitivity>0.2f ||
        !std::isfinite(o.moveSpeed) || o.moveSpeed<0.05f || o.moveSpeed>3.f)
        throw std::runtime_error("Options out of range; see --help");
    if (o.headless && o.frames==0) o.frames=64;
    if (o.scene.empty()) o.scene=o.root/"assets/2cubes/test5.obj";
    if (o.output.empty() && o.headless && !o.selfTest) o.output="render.ppm";
    if (!o.output.empty() && o.output.extension()!=".ppm") throw std::runtime_error("Output must end in .ppm");
    return o;
}

int glErrors=0;
void APIENTRY debugMessage(GLenum,GLenum type,GLuint,GLenum severity,GLsizei,const GLchar* message,const void*) {
    if (type==GL_DEBUG_TYPE_ERROR || severity==GL_DEBUG_SEVERITY_HIGH) {
        ++glErrors; std::cerr<<"OpenGL: "<<message<<"\n";
    }
}
struct Targets {
    GLuint texture[2]{}, fbo[2]{};
    int width=0,height=0,read=0,samples=0;
    ~Targets() { clear(); }
    void clear() {
        glDeleteFramebuffers(2,fbo); glDeleteTextures(2,texture);
        fbo[0]=fbo[1]=texture[0]=texture[1]=0;
    }
    bool resize(int w,int h) {
        if (width==w && height==h) return false;
        clear(); width=w; height=h; samples=0; read=0;
        glGenTextures(2,texture); glGenFramebuffers(2,fbo);
        for (int i=0;i<2;++i) {
            glBindTexture(GL_TEXTURE_2D,texture[i]);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,w,h,0,GL_RGBA,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER,fbo[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture[i],0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("HDR framebuffer incomplete");
            const float zero[4]={0,0,0,1};
            glClearBufferfv(GL_COLOR,0,zero);
        }
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        return true;
    }
};
struct Timers {
    static constexpr int count=8;
    GLuint ids[count]{};
    bool pending[count]{};
    int next=0,active=-1;
    double lastMs=0,totalMs=0;
    size_t measured=0;
    std::vector<double> values;
    Timers() { glGenQueries(count,ids); }
    ~Timers() { glDeleteQueries(count,ids); }
    void collect(int i,bool wait) {
        if (!pending[i]) return;
        GLint available=0;
        glGetQueryObjectiv(ids[i],GL_QUERY_RESULT_AVAILABLE,&available);
        if (!wait && !available) return;
        GLuint64 ns; glGetQueryObjectui64v(ids[i],GL_QUERY_RESULT,&ns);
        lastMs=double(ns)*1e-6; totalMs+=lastMs; ++measured;
        values.push_back(lastMs); pending[i]=false;
    }
    void begin(bool wait) {
        for (int i=0;i<count;++i) collect(i,false);
        collect(next,wait);
        active=pending[next] ? -1 : next;
        if (active>=0) glBeginQuery(GL_TIME_ELAPSED,ids[active]);
    }
    void end() {
        if (active>=0) { glEndQuery(GL_TIME_ELAPSED); pending[active]=true; }
        next=(next+1)%count; active=-1;
    }
    void finish() { for (int i=0;i<count;++i) collect(i,true); }
    double median() const {
        if (values.empty()) return 0;
        auto copy=values; std::sort(copy.begin(),copy.end()); return copy[copy.size()/2];
    }
};
float display(float x,float exposure) {
    x=std::max(x,0.f)*std::exp2(exposure);
    x=std::clamp((x*(2.51f*x+0.03f))/(x*(2.43f*x+0.59f)+0.14f),0.f,1.f);
    return x<=0.0031308f ? 12.92f*x : 1.055f*std::pow(x,1.f/2.4f)-0.055f;
}
std::vector<float> readHDR(const Targets& t) {
    std::vector<float> rgba(size_t(t.width)*t.height*4);
    glBindTexture(GL_TEXTURE_2D,t.texture[t.read]);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,rgba.data());
    for (float v:rgba) if (!std::isfinite(v) || v<0) throw std::runtime_error("Invalid HDR pixel");
    return rgba;
}
void capture(const fs::path& path,const Targets& t,float exposure,const Options& o,
             const Scene& scene,const Timers& timers,double seconds) {
    auto rgba=readHDR(t);
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    fs::path hdr=path; hdr.replace_extension(".pfm");
    fs::path stats=path; stats.replace_extension(".json");
    std::ofstream ppm(path,std::ios::binary), pfm(hdr,std::ios::binary), json(stats);
    if (!ppm || !pfm || !json) throw std::runtime_error("Cannot write capture");
    ppm<<"P6\n"<<t.width<<" "<<t.height<<"\n255\n";
    pfm<<"PF\n"<<t.width<<" "<<t.height<<"\n-1.0\n";
    double sum=0,maxValue=0;
    for (int y=t.height-1;y>=0;--y) for (int x=0;x<t.width;++x) for (int c=0;c<3;++c) {
        float v=rgba[(size_t(y)*t.width+x)*4+c];
        unsigned char b=static_cast<unsigned char>(std::round(display(v,exposure)*255.f));
        ppm.write(reinterpret_cast<const char*>(&b),1);
    }
    for (size_t i=0;i<rgba.size();i+=4) for (int c=0;c<3;++c) {
        float v=rgba[i+c]; pfm.write(reinterpret_cast<const char*>(&v),sizeof(v));
        sum+=v; maxValue=std::max(maxValue,double(v));
    }
    json<<std::setprecision(10)<<"{\n  \"scene\": "<<std::quoted(o.scene.generic_u8string())
        <<", \"attachments\": [";
    for (size_t i=0;i<o.attachments.size();++i) {
        if (i) json<<",";
        json<<std::quoted(o.attachments[i].generic_u8string());
    }
    json<<"], \"gpu\": "<<std::quoted(reinterpret_cast<const char*>(glGetString(GL_RENDERER)))
        <<", \"opengl\": "<<std::quoted(reinterpret_cast<const char*>(glGetString(GL_VERSION)))
        <<",\n  \"width\": "<<t.width<<", \"height\": "<<t.height
        <<", \"spp\": "<<t.samples<<", \"bounces\": "<<o.bounces
        <<", \"seed\": "<<o.seed<<", \"nee\": "<<(o.nee?"true":"false")
        <<", \"mis\": "<<(o.mis?"true":"false")<<", \"bvh\": "<<(o.bvh && (!o.autoTraversal || scene.triangleCount()>64)?"true":"false")
        <<", \"automatic_traversal\": "<<(o.autoTraversal?"true":"false")
        <<", \"uniform_sampling\": "<<(o.uniform?"true":"false")
        <<", \"exposure_ev\": "<<exposure<<", \"environment_radiance\": "<<o.environment
        <<", \"look_sensitivity\": "<<o.lookSensitivity<<", \"move_speed\": "<<o.moveSpeed
        <<", \"debug_view\": "<<o.debugView
        <<",\n  \"triangles\": "<<scene.triangleCount()<<", \"bvh_nodes\": "<<scene.nodeCount()
        <<", \"meshes\": "<<scene.meshCount()<<", \"materials\": "<<scene.materialCount()
        <<", \"color_textures\": "<<scene.colorTextureCount()<<", \"data_textures\": "<<scene.dataTextureCount()
        <<", \"attachment_count\": "<<scene.attachmentCount()
        <<", \"mean_linear_rgb\": "<<sum/(size_t(t.width)*t.height*3)
        <<", \"max_linear_rgb\": "<<maxValue<<",\n  \"gpu_trace_mean_ms\": "<<(timers.measured?timers.totalMs/timers.measured:0)
        <<", \"gpu_trace_median_ms\": "<<timers.median()<<", \"timed_frames\": "<<timers.measured
        <<", \"render_wall_seconds\": "<<seconds<<", \"gl_errors\": "<<glErrors
        <<",\n  \"camera\": { \"position\": ["<<scene.getCamera().position.x<<","<<scene.getCamera().position.y<<","<<scene.getCamera().position.z
        <<"], \"front\": ["<<scene.getCamera().front.x<<","<<scene.getCamera().front.y<<","<<scene.getCamera().front.z
        <<"], \"up\": ["<<scene.getCamera().upDirection.x<<","<<scene.getCamera().upDirection.y<<","<<scene.getCamera().upDirection.z
        <<"], \"fov_degrees\": "<<scene.getCamera().eyeFov<<", \"aspect\": "<<scene.getCamera().aspectRatio<<" }\n}\n";
    if (!ppm || !pfm || !json) throw std::runtime_error("Capture write failed");
    std::cout<<"Saved "<<path.u8string()<<" ("<<t.width<<"x"<<t.height<<", "<<t.samples<<" spp)\n";
}
struct CameraInput {
    explicit CameraInput(GLFWwindow* target) : window(target) {
        previousUser=glfwGetWindowUserPointer(window);
        glfwSetWindowUserPointer(window,this);
        previousCursor=glfwSetCursorPosCallback(window,cursor);
        previousFocus=glfwSetWindowFocusCallback(window,focus);
    }
    ~CameraInput() {
        motion.end();
        looking=false;
        if (glfwGetInputMode(window,GLFW_CURSOR)==GLFW_CURSOR_DISABLED)
            glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        glfwSetCursorPosCallback(window,previousCursor);
        glfwSetWindowFocusCallback(window,previousFocus);
        glfwSetWindowUserPointer(window,previousUser);
    }
    CameraInput(const CameraInput&)=delete;
    CameraInput& operator=(const CameraInput&)=delete;
    bool looking=false;
    bool focusLost=false,requireRelease=false;
    MouseLook motion;
private:
    GLFWwindow* window;
    void* previousUser=nullptr;
    GLFWcursorposfun previousCursor=nullptr;
    GLFWwindowfocusfun previousFocus=nullptr;
    static void cursor(GLFWwindow* window,double x,double y) {
        auto& state=*static_cast<CameraInput*>(glfwGetWindowUserPointer(window));
        if (state.looking) state.motion.position(x,y);
        // A disabled cursor is an unbounded virtual coordinate. Do not send
        // those camera coordinates to ImGui as widget pointer positions.
        else if (state.previousCursor) state.previousCursor(window,x,y);
    }
    static void focus(GLFWwindow* window,int focused) {
        auto& state=*static_cast<CameraInput*>(glfwGetWindowUserPointer(window));
        if (!focused) { state.focusLost=true; state.motion.end(); }
        if (state.previousFocus) state.previousFocus(window,focused);
    }
};
bool applyMouseLook(Camera& camera,MouseLook& motion,float sensitivity,float precision=1.f) {
    const auto delta=motion.consume();
    if (delta.x==0 && delta.y==0) return false;
    camera.yaw(float(-delta.x*sensitivity*precision));
    camera.pitch(float(-delta.y*sensitivity*precision));
    return true;
}
void setMouseLook(GLFWwindow* window,CameraInput& state,bool enabled) {
    state.looking=false;
    state.motion.end();
    // Raw input is optional for GLFW camera control. The bundled Win32 backend
    // can measure absolute WM_INPUT from its recentered OS cursor, so a physical
    // reversal can retain the old delta sign. Standard cursor events avoid it.
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window,GLFW_RAW_MOUSE_MOTION,GLFW_FALSE);
    glfwSetInputMode(window,GLFW_CURSOR,enabled?GLFW_CURSOR_DISABLED:GLFW_CURSOR_NORMAL);
    if (enabled) {
        double x,y; glfwGetCursorPos(window,&x,&y);
        state.motion.begin(x,y);
        state.looking=true;
    }
}
void testMouseLookCallbacks(GLFWwindow* window,CameraInput& state) {
    // Exercise the installed application/ImGui callback chain without moving
    // the user's physical mouse or requiring a particular input device.
    auto callback=glfwSetCursorPosCallback(window,nullptr);
    glfwSetCursorPosCallback(window,callback);
    if (!callback) throw std::runtime_error("Camera cursor callback missing");
    auto guiFrame=[&]() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::EndFrame();
    };
    Camera camera;
    setMouseLook(window,state,true);
    guiFrame();
    if (glfwGetInputMode(window,GLFW_CURSOR)!=GLFW_CURSOR_DISABLED)
        throw std::runtime_error("ImGui changed camera cursor mode");
    if (glfwRawMouseMotionSupported() && glfwGetInputMode(window,GLFW_RAW_MOUSE_MOTION))
        throw std::runtime_error("Camera unexpectedly enabled raw input");
    double x,y; glfwGetCursorPos(window,&x,&y);
    if (applyMouseLook(camera,state.motion,0.04f))
        throw std::runtime_error("Camera moved on capture");
    callback(window,x+40,y);
    guiFrame();
    if (!applyMouseLook(camera,state.motion,0.04f) || camera.front.x<=0)
        throw std::runtime_error("Camera did not turn right");
    float rightX=camera.front.x;
    // Still right of the press point, but left of the last position.
    callback(window,x+30,y);
    guiFrame();
    if (!applyMouseLook(camera,state.motion,0.04f) || camera.front.x>=rightX)
        throw std::runtime_error("Camera did not reverse left");
    const auto stopped=camera.front;
    for (int i=0;i<4;++i) {
        guiFrame();
        if (applyMouseLook(camera,state.motion,0.04f) || glm::length(camera.front-stopped)>1e-7f)
            throw std::runtime_error("Camera repeated stale mouse motion");
    }
    callback(window,x+30,y-20);
    guiFrame();
    applyMouseLook(camera,state.motion,0.04f);
    float upY=camera.front.y;
    callback(window,x+30,y-10);
    guiFrame();
    applyMouseLook(camera,state.motion,0.04f);
    if (upY<=0 || camera.front.y>=upY) throw std::runtime_error("Camera did not reverse vertically");
    callback(window,x+80,y); // Unconsumed movement must be discarded on release.
    setMouseLook(window,state,false);
    setMouseLook(window,state,true);
    if (applyMouseLook(camera,state.motion,0.04f))
        throw std::runtime_error("Camera replayed movement from previous drag");
    setMouseLook(window,state,false);
}
bool input(GLFWwindow* window,Scene& scene,float dt,bool allowKeyboard,bool allowLook,
           CameraInput& state,float sensitivity,float moveSpeed) {
    Camera& c=scene.getCamera();
    bool changed=false;
    bool focused=glfwGetWindowAttrib(window,GLFW_FOCUSED)==GLFW_TRUE;
    bool held=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
    if (state.focusLost) { state.requireRelease=true; state.focusLost=false; }
    if (!held) state.requireRelease=false;
    // Once captured, keep the drag until release even if ImGui later requests
    // mouse input. Only a press inside the scene may start a new camera drag.
    bool wantLook=focused && held && !state.requireRelease && (state.looking || allowLook);
    if (wantLook!=state.looking) setMouseLook(window,state,wantLook);
    if (state.looking) {
        // Mouse displacement already integrates motion; do not multiply by dt.
        float precision=glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS?0.25f:1.0f;
        changed=applyMouseLook(c,state.motion,sensitivity,precision);
    }
    if (focused && allowKeyboard && dt>0.f) {
        vec3 direction(0);
        if (glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS) direction.z+=1;
        if (glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS) direction.z-=1;
        if (glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS) direction.x+=1;
        if (glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS) direction.x-=1;
        if (glfwGetKey(window,GLFW_KEY_E)==GLFW_PRESS) direction.y+=1;
        if (glfwGetKey(window,GLFW_KEY_Q)==GLFW_PRESS) direction.y-=1;
        if (glm::length(direction)>0) {
            float modifier=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS?3.f:1.f;
            if (glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) modifier*=0.25f;
            c.move(direction,dt,moveSpeed*modifier);
            changed=true;
        }
    }
    return changed;
}
int run(GLFWwindow* window,Options o) {
    Shader tracer((o.root/"shaders/pathtracing.vert").u8string().c_str(),(o.root/"shaders/pathtracing.frag").u8string().c_str());
    Shader present((o.root/"shaders/pathtracing.vert").u8string().c_str(),(o.root/"shaders/display.frag").u8string().c_str());
    std::vector<std::string> attachments;
    for (const auto& path:o.attachments) attachments.push_back(path.u8string());
    Scene scene(o.scene.u8string().c_str(),attachments);
    // Upload once, outside GPU trace measurements.
    tracer.use();
    // The initial draw uploads buffers outside the measured loop.
    scene.prepare(tracer);
    // Check the actual driver's std430 layout against CPU serialization.
    struct Layout { const char* name; int stride; };
    const Layout layouts[]={{"lights[0].center",80},{"materials[0].baseColor",32},
        {"triangles_Data[0].materialIdx",100},{"nodes[0].mi",48},{"textureMaterials[0].maps0",48}};
    for (const auto& layout:layouts) {
        GLuint index=glGetProgramResourceIndex(tracer.ID,GL_BUFFER_VARIABLE,layout.name);
        if (index==GL_INVALID_INDEX) throw std::runtime_error(std::string("Missing SSBO variable: ")+layout.name);
        GLenum property=GL_TOP_LEVEL_ARRAY_STRIDE;
        GLint stride=0;
        glGetProgramResourceiv(tracer.ID,GL_BUFFER_VARIABLE,index,1,&property,1,nullptr,&stride);
        if (stride!=layout.stride) throw std::runtime_error(std::string("SSBO stride mismatch: ")+layout.name);
    }
    if (o.selfTest) {
        GLuint id=0;
        {
            float value=1;
            SSBO a("lifetime-test",tracer.ID,sizeof(value),5);
            a.bindData(&value);
            GLint bound=0; glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING,5,&bound); id=GLuint(bound);
            SSBO b(std::move(a));
            SSBO c; c=std::move(b);
            if (!glIsBuffer(id)) throw std::runtime_error("Moved SSBO deleted too soon");
            c.bindData(&value);
        }
        if (glIsBuffer(id)) throw std::runtime_error("SSBO resource leaked");
    }
    Targets targets;
    Timers timers;
    bool paused=false, adaptive=true, captureRequested=false;
    CameraInput controls(window);
    if (o.selfTest) testMouseLookCallbacks(window,controls);
    bool wasMoving=false;
    int targetSamples=4096, frames=0;
    double previous=glfwGetTime();
    double idleSince=previous, start=previous;
    Camera initial=scene.getCamera();
    std::cout<<"Scene: "<<o.scene.u8string()<<"\n";
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS) break;
        double now=glfwGetTime();
        float dt=float(std::clamp(now-previous,0.0,0.1)); previous=now;
        bool dirty=false;
        if (!o.headless) {
            ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        }
        int width,height; glfwGetFramebufferSize(window,&width,&height);
        if (!o.headless && (width==0 || height==0)) { ImGui::EndFrame(); glfwWaitEventsTimeout(0.05); continue; }
        int panelWidth=o.headless ? 0 : std::min(360,std::max(0,width*2/5));
        int viewWidth=o.headless?o.width:std::max(1,width-panelWidth);
        int viewHeight=o.headless?o.height:std::max(1,height);
        bool moved=false;
        if (!o.headless) {
            int logicalW,logicalH;
            glfwGetWindowSize(window,&logicalW,&logicalH);
            double cursorX,cursorY;
            glfwGetCursorPos(window,&cursorX,&cursorY);
            double sceneRight=double(viewWidth)*logicalW/std::max(width,1);
            bool inScene=cursorX>=0 && cursorX<sceneRight && cursorY>=0 && cursorY<logicalH;
            const auto& io=ImGui::GetIO();
            bool keyboard=!io.WantCaptureKeyboard && (!io.WantCaptureMouse || controls.looking);
            bool look=!io.WantCaptureMouse && inScene;
            moved=input(window,scene,dt,keyboard,look,controls,o.lookSensitivity,o.moveSpeed);
        }
        if (moved) { idleSince=now; dirty=true; }
        bool moving=!o.headless && adaptive && (moved || now-idleSince<0.15);
        if (wasMoving && !moving) dirty=true;
        wasMoving=moving;
        if (!o.headless) {
            int logicalW,logicalH; glfwGetWindowSize(window,&logicalW,&logicalH);
            float sx=float(logicalW)/std::max(width,1), sy=float(logicalH)/std::max(height,1);
            ImGui::SetNextWindowPos(ImVec2(viewWidth*sx,0));
            ImGui::SetNextWindowSize(ImVec2(panelWidth*sx,float(logicalH)));
            ImGui::Begin("Path tracer",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Progressive HDR path tracing");
            ImGui::Separator();
            ImGui::Text("%.1f FPS | GPU %.2f ms",ImGui::GetIO().Framerate,timers.lastMs);
            ImGui::Text("%d spp | %d x %d",targets.samples,targets.width,targets.height);
            ImGui::Text("%zu triangles | %zu BVH nodes",scene.triangleCount(),scene.nodeCount());
            ImGui::Text("%zu materials | %zu color + %zu data maps",
                scene.materialCount(),scene.colorTextureCount(),scene.dataTextureCount());
            ImGui::TextUnformatted(o.bvh && (!o.autoTraversal || scene.triangleCount()>64)?"Traversal: BVH":"Traversal: direct (small scene)");
            ImGui::TextUnformatted(paused?"Paused":moving?"Moving preview":
                targetSamples>0 && targets.samples>=targetSamples?"Target reached":"Accumulating");
            if (ImGui::Button("Restart")) dirty=true;
            ImGui::SameLine(); ImGui::Checkbox("Pause",&paused);
            if (ImGui::Button("Reset camera")) { scene.getCamera()=initial; dirty=true; }
            ImGui::SameLine();
            if (ImGui::Button("Save PPM + HDR")) captureRequested=true;
            ImGui::PushItemWidth(180);
            dirty|=ImGui::SliderInt("Bounces",&o.bounces,1,16);
            dirty|=ImGui::SliderFloat("Render scale",&o.scale,0.25f,1.f,"%.2f");
            ImGui::Checkbox("Adaptive preview",&adaptive);
            ImGui::SliderFloat("Exposure (EV)",&o.exposure,-6,6);
            const char* views[]={"Path tracing","Base color","Shading normal"};
            dirty|=ImGui::Combo("View",&o.debugView,views,IM_ARRAYSIZE(views));
            dirty|=ImGui::SliderFloat("Environment",&o.environment,0,0.5f,"%.3f");
            ImGui::InputInt("Target spp",&targetSamples);
            targetSamples=std::clamp(targetSamples,0,1000000);
            ImGui::TextUnformatted("0 = continue (up to 16M spp)");
            dirty|=ImGui::Checkbox("Direct light sampling",&o.nee);
            dirty|=ImGui::Checkbox("MIS",&o.mis);
            if (ImGui::CollapsingHeader("Diagnostics")) {
                if (ImGui::Checkbox("Auto traversal",&o.autoTraversal)) {
                    dirty=true;
                    if (o.autoTraversal) o.bvh=true;
                }
                if (!o.autoTraversal) dirty|=ImGui::Checkbox("BVH traversal",&o.bvh);
                dirty|=ImGui::Checkbox("Uniform hemisphere",&o.uniform);
            }
            if (ImGui::CollapsingHeader("Camera controls")) {
                ImGui::SliderFloat("Look speed",&o.lookSensitivity,0.005f,0.2f,"%.3f");
                ImGui::SliderFloat("Move speed",&o.moveSpeed,0.05f,3.0f,"%.2f");
                if (ImGui::Button("Default controls")) {
                    o.lookSensitivity=0.04f; o.moveSpeed=0.5f;
                }
                ImGui::TextWrapped("Ctrl: quarter speed for precise mouse and keyboard movement.");
            }
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::TextWrapped("Hold right mouse to look. WASD move, Q/E down/up, Shift accelerate, Ctrl fine control. Esc exits.");
            ImGui::TextWrapped("Camera and lighting changes restart accumulation. Exposure keeps HDR samples.");
            ImGui::End();
        }
        float scale=moving?std::min(o.scale,0.5f):o.scale;
        int renderW=std::max(1,int(std::round(viewWidth*scale)));
        int renderH=std::max(1,int(std::round(viewHeight*scale)));
        if (targets.resize(renderW,renderH)) dirty=true;
        Camera& camera=scene.getCamera();
        float aspect=float(viewWidth)/viewHeight;
        if (camera.aspectRatio!=aspect) { camera.aspectRatio=aspect; dirty=true; }
        if (o.selfTest) {
            // Exercise the same resize/reset paths used by the UI.
            if (frames==3) { camera.position.x+=camera.speed*0.1f; dirty=true; }
            if (frames==6) { targets.resize(renderW+7,renderH+3); dirty=true; }
            if (frames==9) { camera=initial; camera.aspectRatio=aspect; dirty=true; }
        }
        if (dirty) targets.samples=0;
        if ((!paused || dirty) && (o.headless || targetSamples==0 || targets.samples<targetSamples) && targets.samples<16000000) {
            int write=1-targets.read;
            glBindFramebuffer(GL_FRAMEBUFFER,targets.fbo[write]); glViewport(0,0,targets.width,targets.height);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,targets.texture[targets.read]);
            tracer.use();
            tracer.setInt("previousFrame",0); tracer.setInt("frameCount",targets.samples);
            tracer.setInt("maxBounces",o.bounces); tracer.setInt("rngSeed",o.seed);
            tracer.setInt("debugView",o.debugView);
            tracer.setBool("useNEE",o.nee); tracer.setBool("useMIS",o.mis);
            tracer.setBool("useBVH",o.bvh && (!o.autoTraversal || scene.triangleCount()>64));
            tracer.setBool("uniformSampling",o.uniform);
            tracer.setFloat("rayEpsilon",scene.epsilon()); tracer.setFloat("environmentStrength",o.environment);
            timers.begin(o.headless);
            scene.draw(tracer);
            timers.end();
            targets.read=write; ++targets.samples;
            if (o.selfTest && dirty && targets.samples!=1) throw std::runtime_error("History reset failed");
        }
        if (!o.headless) {
            glBindFramebuffer(GL_FRAMEBUFFER,0);
            glViewport(0,0,width,height); glClearColor(0.025f,0.025f,0.025f,1); glClear(GL_COLOR_BUFFER_BIT);
            glViewport(0,0,viewWidth,viewHeight);
            present.use(); present.setInt("image",0); present.setFloat("exposure",o.exposure);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,targets.texture[targets.read]);
            scene.drawQuad();
            glViewport(0,0,width,height);
            ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
        if (captureRequested) {
            timers.finish();
            auto stamp=std::chrono::system_clock::now().time_since_epoch().count();
            capture(o.root/"captures"/("capture-"+std::to_string(stamp)+".ppm"),targets,o.exposure,o,scene,timers,now-start);
            captureRequested=false;
        }
        ++frames;
        if (o.frames>0 && frames>=o.frames) break;
        if (!o.headless && (paused || (targetSamples>0 && targets.samples>=targetSamples)))
            glfwWaitEventsTimeout(0.016);
    }
    timers.finish();
    if (o.selfTest) {
        readHDR(targets);
        if (targets.samples!=3) throw std::runtime_error("Unexpected final history sample count");
        std::cout<<"SELF_TEST_PASS: cursor reversal/stop/recapture callbacks, camera/resize/reset/HDR readback\n";
    }
    if (!o.output.empty() && targets.width>0)
        capture(o.output,targets,o.exposure,o,scene,timers,glfwGetTime()-start);
    std::cout<<"Trace GPU median "<<timers.median()<<" ms, mean "
        <<(timers.measured?timers.totalMs/timers.measured:0)<<" ms\n";
    if (glErrors) throw std::runtime_error("OpenGL validation errors: "+std::to_string(glErrors));
    return 0;
}
int main(int argc,char** argv) {
    GLFWwindow* window=nullptr;
    bool ui=false;
    try {
        Options o=parse(commandLine(argc,argv));
        glfwSetErrorCallback([](int code,const char* message) { std::cerr<<"GLFW "<<code<<": "<<message<<"\n"; });
        if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE,o.headless?GLFW_FALSE:GLFW_TRUE);
        window=glfwCreateWindow(o.width,o.height,"Progressive Path Tracer",nullptr,nullptr);
        if (!window) throw std::runtime_error("OpenGL 4.3 context required");
        glfwMakeContextCurrent(window);
        if (!o.headless) glfwSetWindowSizeLimits(window,900,600,GLFW_DONT_CARE,GLFW_DONT_CARE);
        // Event callbacks preserve short key presses between rendered frames.
        glfwSetKeyCallback(window,[](GLFWwindow* w,int key,int,int action,int) {
            if (key==GLFW_KEY_ESCAPE && action==GLFW_PRESS) glfwSetWindowShouldClose(w,GLFW_TRUE);
        });
        glfwSetInputMode(window,GLFW_STICKY_KEYS,GLFW_TRUE);
        if (!gladLoadGL(glfwGetProcAddress) || !GLAD_GL_VERSION_4_3)
            throw std::runtime_error("OpenGL 4.3 unavailable");
        std::cout<<"GPU: "<<glGetString(GL_RENDERER)<<" | "<<glGetString(GL_VERSION)<<"\n";
        glEnable(GL_DEBUG_OUTPUT); glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); glDebugMessageCallback(debugMessage,nullptr);
        glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_FRAMEBUFFER_SRGB);
        glfwSwapInterval(o.headless?0:1);
        if (!o.headless || o.selfTest) {
            IMGUI_CHECKVERSION(); ImGui::CreateContext(); ui=true;
            ImGui::GetIO().IniFilename=nullptr;
            ImFontConfig font;
            font.SizePixels=18.0f;
            ImGui::GetIO().Fonts->AddFontDefault(&font);
            ImGui::StyleColorsDark();
            ImGui_ImplGlfw_InitForOpenGL(window,true); ImGui_ImplOpenGL3_Init("#version 430 core");
        }
        int result=run(window,o);
        if (ui) { ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); }
        glfwDestroyWindow(window); glfwTerminate();
        return result;
    } catch (const std::exception& e) {
        std::cerr<<"ERROR: "<<e.what()<<"\n";
        if (ui) { ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); }
        if (window) glfwDestroyWindow(window);
        glfwTerminate(); return 1;
    }
}
