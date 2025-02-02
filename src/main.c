#include "bitmap.h"
#include "renderer.h"
#include "scene.h"
#include "threadpool.h"
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

/*
int main() {
    // Seed rng
    srandom(time(NULL));

    // Define the scene
    scene world;
    scene_init(&world);

    // world.sky_color = (vec3s){135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};
    world.sky_color = glms_vec3_broadcast(0.0);

    int sun =
        scene_add_material(&world, (vec3s){0.9372, 0.7490, 0.0157}, 0.2, 1.0,
                           (vec3s){0.9372, 0.7490, 0.0157}, 1.0, 0.0);
    int red_plastic = scene_add_material(&world, (vec3s){1, 0, 0}, 0.5, 0.5,
                                         (vec3s){1, 0, 0}, 0.0, 0.0);
    int green_grass = scene_add_material(&world, (vec3s){0.0, 1.0, 0.0}, 1.0,
                                         0.1, (vec3s){0.0, 1.0, 0.0}, 0.0, 0.0);
    int mirror = scene_add_material(&world, (vec3s){1, 1, 1}, 0.0, 1.0,
                                    glms_vec3_zero(), 0.0, 0.0);
    int glass = scene_add_material(&world, (vec3s){1.0, 1.0, 1.0}, 0.0, 0.0,
                                   (vec3s){1.0, 1.0, 1.0}, 0.0, 1.0);

    scene_add_sphere(&world, (vec3s){25.0, 25.0, 40}, 25, sun);
    scene_add_sphere(&world, (vec3s){-0.5, 0.0, 5.0}, 1, red_plastic);
    scene_add_sphere(&world, (vec3s){0.5, -0.5, 2.5}, 0.5, glass);
    // scene_add_sphere(&world, (vec3s){0.0, -51.0, 5.0}, 50, green_grass);

    // scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){50, -1, -50},
    //                    (vec3s){50, -1, 50}, green_grass);
    // scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){-50, -1, 50},
    //                    (vec3s){50, -1, 50}, green_grass);

    // scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){-2.5, -2.5,
    // 10},
    //                    (vec3s){2.5, -2.5, 10}, mirror);
    // scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){2.5, 2.5, 10},
    //                    (vec3s){2.5, -2.5, 10}, mirror);

    // scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){-5, -2.5, -10},
    //                    (vec3s){5, -2.5, -10}, mirror);
    // scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){5, 2.5, -10},
    //                    (vec3s){5, -2.5, -10}, mirror);

// Initialize frame and fill it with black
uint8_t frame[WIDTH * HEIGHT * 3];
for (int i = 0; i < WIDTH * HEIGHT; i++) {
    frame[3 * i + 0] = 0;
    frame[3 * i + 1] = 0;
    frame[3 * i + 2] = 0;
}

// Render the scene
threadpool pool;
threadpool_init(&pool, 8);
render_args thread_args[HEIGHT];
for (int y = 0; y < HEIGHT; y++) {
    thread_args[y].screen_y = y;
    thread_args[y].framew = WIDTH;
    thread_args[y].frameh = HEIGHT;
    thread_args[y].frame = frame;
    thread_args[y].world = &world;

    threadpool_add_task(&pool, (void *)(void *)render_task, &thread_args[y]);
    // render_task(&thread_args[y]);
}
threadpool_wait_for_tasks(&pool);
threadpool_destroy(&pool);

// Write rendered scene to an image
write_bitmap("output.bmp", WIDTH, HEIGHT, frame);

// Cleanup
scene_destroy(&world);

return 0;
}
*/

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

    // Load shader sources
    size_t buffer_size = 4096;
    char buffer[buffer_size];
    GLint compile_status;

    FILE *vert_fp = fopen("shaders/triangle_vert.glsl", "r");
    int vert_len = fread(buffer, sizeof(char), buffer_size - 1, vert_fp);
    fclose(vert_fp);
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vert_source = buffer;
    glShaderSource(vertex_shader, 1, &vert_source, &vert_len);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLint info_log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        char info_buffer[info_log_length];
        glGetShaderInfoLog(vertex_shader, info_log_length, NULL, info_buffer);
        fprintf(stderr, "Failed to compile vertex shader:\n%s", info_buffer);
        return 1;
    }

    FILE *frag_fp = fopen("shaders/triangle_frag.glsl", "r");
    int frag_len = fread(buffer, sizeof(char), buffer_size - 1, frag_fp);
    fclose(frag_fp);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *frag_source = buffer;
    glShaderSource(fragment_shader, 1, &frag_source, &frag_len);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLint info_log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        char info_buffer[info_log_length];
        glGetShaderInfoLog(fragment_shader, info_log_length, NULL, info_buffer);
        fprintf(stderr, "Failed to compile fragment shader:\n%s", info_buffer);
        return 1;
    }

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vertex_shader);
    glAttachShader(shader, fragment_shader);
    glLinkProgram(shader);

    glGetProgramiv(shader, GL_LINK_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLint info_log_length;
        glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        char info_buffer[info_log_length];
        glGetProgramInfoLog(shader, info_log_length, NULL, info_buffer);
        fprintf(stderr, "Failed to link shader program:\n%s", info_buffer);
        return 1;
    }

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
