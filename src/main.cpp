#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shaderManager.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tools.h"
#include "camera.h"
#include "model.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int moveState[4];
glm::vec3 moveArrow;

int main() {
    // glfw init
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
    // load glad
    int version = gladLoadGL(glfwGetProcAddress);
    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    // set init state and callback function
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // my shader
    Shader shader("C:/Users/solaryan/Desktop/CG/raytracing/src/shaders/vertexshader.vert", "C:/Users/solaryan/Desktop/CG/raytracing/src/shaders/pathtracing.frag");
    Scene Xmodel("C:/Users/solaryan/Desktop/CG/raytracing/assets/2cubes/test4.obj");

    std::cout << "------ Begin ------" << std::endl;

    float last_time = glfwGetTime(), detaTime;
	float fps = 60, counter_time = 0, counter_frame = 0;

    // ---------- render loop ----------
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
        glClear(GL_COLOR_BUFFER_BIT);
        // 使用着色器程序，使用后才能 set
        shader.use();
        
        shader.setFloat("iTime", detaTime - int(detaTime));

        Xmodel.draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
        // break;
    }

    // ------ release ------
    shader.del();
    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W) moveState[0] = action;
    if (key == GLFW_KEY_S) moveState[1] = action;
    if (key == GLFW_KEY_D) moveState[2] = action;
    if (key == GLFW_KEY_A) moveState[3] = action;
    moveArrow.z = 1.0f * moveState[0] - moveState[1];
    moveArrow.x = 1.0f * moveState[2] - moveState[3];
    // std::cout << moveArrow.x << " " << moveArrow.y << " " << moveArrow.z << std::endl;
    // camera.move(moveArrow);
    // if (key == GLFW_KEY_Q && action == GLFW_REPEAT) {
    //     camera.yaw(camera.angularVelocity);
    // } 
    // if (key == GLFW_KEY_E && action == GLFW_REPEAT) {
    //     camera.yaw(-camera.angularVelocity);
    // }
    // if (key == GLFW_KEY_SPACE && action == GLFW_REPEAT) {
    //     camera.up();
    // }
    // if (key == GLFW_KEY_SPACE && mods == GLFW_MOD_SHIFT) {
    //     camera.down();
    // }
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    //     glfwSetWindowShouldClose(window, GL_TRUE);
    // }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    int r = std::min(width, height);
    glViewport(0, height - r, r, r);
}