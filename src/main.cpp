#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shaderManager.h"
#include "stb_image.h"
#include <glm/glm.hpp>

#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

float texCoords[] = {
    0.0f, 0.0f, // 左下角
    1.0f, 0.0f, // 右下角
    0.5f, 1.0f  // 上中
};

// 三角形的顶点数据
float vertices[] = {
    -0.433f, -0.25f, 0.0f, 1.0f, 0.0f, 0.0f, // 左下角
    0.433f , -0.25f, 0.0f, 0.0f, 1.0f, 0.0f, // 右下角
    0.0f ,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // 上方中间
};

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
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
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
    glViewport(0, 0, 600, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // my shader
    Shader shader("src\\shaders\\vertexshader.glsl", "src\\shaders\\fragmentshader.glsl");

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // transfer data
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // set VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);  


    float angle = 3.1415926535 / 120, cur = 0;

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // 使用着色器程序
        shader.use();

        // -- add --
        float matrix[] = {
            cos(cur), -sin(cur), 0, 0,
            sin(cur),  cos(cur), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        glm::mat4 rotate = glm::make_mat4(matrix);
        shader.setMat4("rotate", rotate);
        cur += angle;
        // -- end --
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release data
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shader.del();
    glfwTerminate();


    return 0;
}

void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}