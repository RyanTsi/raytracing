#include <iostream>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "config.h"
#include "./model/scene.h"
// #include "uiManager.h"
#include "shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main () {
    // ------ glfw init ------
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create a window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Renderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    } 
    glfwMakeContextCurrent(window);

    
    // ------ load glad ------
    int version = gladLoadGL(glfwGetProcAddress);
    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");


    // set init State and Callback Function
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    Shader shader("C:/Users/solaryan/Desktop/CG/raytracing/shaders/pathtracing.vert", "C:/Users/solaryan/Desktop/CG/raytracing/shaders/pathtracing.frag");
    Scene scene("C:/Users/solaryan/Desktop/CG/raytracing/assets/2cubes/test5.obj");

    std::cout << "------ Begin ------" << std::endl;
    float last_time = glfwGetTime(), detaTime;
	float fps = 60, counter_time = 0, counter_frame = 0;

    // 设置帧缓冲
    std::vector<glm::vec3> frameBuffer(SCREEN_WIDTH * SCREEN_HEIGHT, glm::vec3(0.0f)); // 用于累积颜色值
    int frameCount = 0; // 记录已渲染的帧数
    GLuint previousFrameTexture;
    glGenTextures(1, &previousFrameTexture);
    glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    static char inputText[256] = "6";
    shader.use();
    shader.setInt("A", 6);
    while (!glfwWindowShouldClose(window)) {
        // ------ record ------
        detaTime = glfwGetTime() - last_time;
        last_time += detaTime;
        counter_frame ++;
        counter_time += detaTime;
        if(counter_time > 1) {
            fps = counter_frame / counter_time, counter_frame = counter_time = 0;
            std::cout << fps << std::endl;
        }
        // ------ record ------

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();

        // 添加随机抖动
        float jitterX = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float jitterY = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        glm::mat4 jitterMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(jitterX / SCREEN_WIDTH, jitterY / SCREEN_HEIGHT, 0.0f));

        // 将抖动矩阵传递给着色器
        shader.setMat4("jitterMatrix", jitterMatrix);

        // 绑定上一帧的累积结果
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
        glUniform1i(glGetUniformLocation(shader.ID, "previousFrame"), 0);

        shader.setInt("frameCount", frameCount);
        shader.setFloat("iTime", detaTime - int(detaTime));
        
        scene.draw(shader);

         // 将当前帧的结果保存回纹理
        glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        if(frameCount < 2000) {
            frameCount ++;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a simple window
        ImGui::Begin("imgui");
        ImGui::Text("FPS:%.2f", fps);
        ImGui::Text("SPPCNT:%d", frameCount);
        if (ImGui::Button("fresh")) {
            // 按钮被点击时执行的代码
            // std::cout << "Button was clicked!" << std::endl;
            frameBuffer = std::vector<glm::vec3>(SCREEN_WIDTH * SCREEN_HEIGHT, glm::vec3(0.0f));
            frameCount  = 0;
        }
        // 添加一个文本框
        if (ImGui::InputText("recursions", inputText, IM_ARRAYSIZE(inputText))) {
            // 文本框内容发生变化时执行的代码
            // std::cout << "User input: " << inputText << std::endl;
            if(strlen(inputText) != 0) {
                shader.setInt("A", inputText[0] - '0');
                frameBuffer = std::vector<glm::vec3>(SCREEN_WIDTH * SCREEN_HEIGHT, glm::vec3(0.0f));
                frameCount  = 0;
            }
        }
        ImGui::End();

        // Rendering ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    
    return 0;
}