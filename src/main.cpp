#include <iostream>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "config.h"
#include "./modelManager/scene.h"
#include "shader.h"

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

    // set init State and Callback Function
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    Shader shader("C:/Users/solaryan/Desktop/trace/shaders/pathtracing.vert", "C:/Users/solaryan/Desktop/trace/shaders/pathtracing.frag");
    Scene scene("C:/Users/solaryan/Desktop/CG/raytracing/assets/2cubes/test4.obj");

    std::cout << "------ Begin ------" << std::endl;
    float last_time = glfwGetTime(), detaTime;
	float fps = 60, counter_time = 0, counter_frame = 0;

    
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

        shader.setFloat("iTime", detaTime - int(detaTime));
        scene.draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    
    return 0;
}