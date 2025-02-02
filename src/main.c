#include "shader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <cglm/struct.h>
#include <glad/gl.h>

#define WIDTH 1280
#define HEIGHT 800

void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }
    glfwSetErrorCallback(error_callback);

    // GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a window
    GLFWwindow *window =
        glfwCreateWindow(WIDTH, HEIGHT, "Path Tracer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Load OpenGL function pointers
    gladLoadGL(glfwGetProcAddress);

    // Build shader
    int shader = build_shader("shaders/triangle_vert.glsl",
                              "shaders/triangle_frag.glsl");
    if (shader == -1)
        return 1;

    // Create vertex array and buffer
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float vertices[15] = {0.0, 0.5, 1.0, 0.0,  0.0, -0.5, -0.5, 0.0,
                          1.0, 0.0, 0.5, -0.5, 0.0, 0.0,  1.0};
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 1.0);

        glUseProgram(shader);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}
