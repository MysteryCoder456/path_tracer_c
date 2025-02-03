#include "gpu/shader.h"
#include <math.h>
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
    int shader = build_shader("shaders/rtx_vert.glsl", "shaders/rtx_frag.glsl");
    if (shader == -1)
        return 1;
    int aspect_ratio_loc = glGetUniformLocation(shader, "aspect_ratio");
    int fov_loc = glGetUniformLocation(shader, "fov");

    // Create vertex array and buffer
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float vertices[12] = {
        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0,
    };
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float fov = 90.0 * M_PI / 180.0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 1.0);

        glUseProgram(shader);
        glBindVertexArray(vao);
        glUniform1f(aspect_ratio_loc, (float)width / (float)(height));
        glUniform1f(fov_loc, fov);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}
