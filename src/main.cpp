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

// 三角形的顶点数据
float vertices[] = {
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
     5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
     5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
     5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
    -5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f,  0.0f, -1.0f,
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 0
    -5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
     5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
    -5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f,  0.0f,  1.0f,
    -5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 1
    -5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
    -5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
    -5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
    -5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 2
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
     5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 3
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
     5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
     5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
     5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
    -5.0f, -5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
    -5.0f, -5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 4
    -5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
     5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
     5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
    -5.0f,  5.0f, -20.0f, 1.0f, 0.5f, 0.31f, 0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
    -5.0f,  5.0f, -30.0f, 1.0f, 0.5f, 0.31f, 0.0f, 1.0f,  0.0f,  1.0f,  0.0f,  // 5
};
    
glm::vec3 lightPos(10, 10, -20), lightColor(1.0f, 1.0f, 1.0f);

Camera camera;
int moveState[4];
glm::vec3 moveArrow;


int main() {
    // glfw init
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_DEPTH_TEST);
    // my shader
    Shader shader("src\\shaders\\vertexshader.glsl", "src\\shaders\\fragmentshader.glsl");
    
    // texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // 为当前绑定的纹理对象设置环绕、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 加载并生成纹理
    int width, height, nrChannels;
    unsigned char *data = stbi_load("C:/Users/solaryan/Desktop/CG/raytracing/assets/image.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // transfer data
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // set VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    Model model("C:\\Users\\solaryan\\Desktop\\CG\\test\\assets\\test.obj");
    double last_time = glfwGetTime(), detaTime;
	double fps = 60, counter_time = 0, counter_frame = 0;
    // render loop
    while (!glfwWindowShouldClose(window)) {
        detaTime = glfwGetTime() - last_time;
        last_time += detaTime;
        counter_frame ++;
        counter_time += detaTime;
        if(counter_time > 1) {
            fps = counter_frame / counter_time, counter_frame = counter_time = 0;
            std::cout << fps << std::endl;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        // 使用着色器程序，使用后才能 set
        shader.use();

        // -- add --
        shader.setMat4("project", camera.getProjectionMatrix());
        shader.setMat4("view", camera.getViewMatrix());
        glm::mat4 model(1.0f);
        shader.setMat4("model", model);
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("lightColor", lightColor);
        // -- end --
        
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release data
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(2, &EBO);
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
    camera.move(moveArrow);
    if (key == GLFW_KEY_Q && action == GLFW_REPEAT) {
        camera.yaw(camera.angularVelocity);
    } 
    if (key == GLFW_KEY_E && action == GLFW_REPEAT) {
        camera.yaw(-camera.angularVelocity);
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_REPEAT) {
        camera.up();
    }
    if (key == GLFW_KEY_SPACE && mods == GLFW_MOD_SHIFT) {
        camera.down();
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    int r = std::min(width, height);
    glViewport(0, height - r, r, r);
}