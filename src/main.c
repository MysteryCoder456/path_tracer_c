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

int main() {
    // Seed rng
    srandom(time(NULL));

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }
    glfwSetErrorCallback(error_callback);

    // Create a window
    GLFWwindow *window =
        glfwCreateWindow(WIDTH, HEIGHT, "Path Tracer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        return -1;
    }

    // Additional GLFW options
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Load OpenGL function pointers
    gladLoadGL(glfwGetProcAddress);

    // Define the scene
    scene world;
    scene_init(&world);

    /*world.sky_color = (vec3s){135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};*/
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
    /*scene_add_sphere(&world, (vec3s){0.0, -51.0, 5.0}, 50, green_grass);*/

    scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){50, -1, -50},
                       (vec3s){50, -1, 50}, green_grass);
    scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){-50, -1, 50},
                       (vec3s){50, -1, 50}, green_grass);

    /*
    scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){-2.5, -2.5,
    10},
                       (vec3s){2.5, -2.5, 10}, mirror);
    scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){2.5, 2.5, 10},
                       (vec3s){2.5, -2.5, 10}, mirror);

    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){-5, -2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);
    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){5, 2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);
    */

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

        threadpool_add_task(&pool, (void *)(void *)render_task,
                            &thread_args[y]);
        // render_task(&thread_args[y]);
    }
    threadpool_wait_for_tasks(&pool);
    threadpool_destroy(&pool);

    // Write rendered scene to an image
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    scene_destroy(&world);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
