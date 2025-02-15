#include "bitmap.h"
#include "gpu/shader.h"
#include "scene.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void uniform_materials(scene *world, int program) {
    glUseProgram(program);

    for (int i = 0; i < world->num_materials; i++) {
        shape_material mat = world->materials[i];

        char identifier[32];
        sprintf(identifier, "materials[%d].", i);

        char albedo[64];
        strcpy(albedo, identifier);
        strcat(albedo, "albedo");
        glUniform3fv(glGetUniformLocation(program, albedo), 1, mat.albedo.raw);

        char roughness[64];
        strcpy(roughness, identifier);
        strcat(roughness, "roughness");
        glUniform1f(glGetUniformLocation(program, roughness), mat.roughness);

        char metallicity[64];
        strcpy(metallicity, identifier);
        strcat(metallicity, "metallicity");
        glUniform1f(glGetUniformLocation(program, metallicity),
                    mat.metallicity);

        char emission_color[64];
        strcpy(emission_color, identifier);
        strcat(emission_color, "emission_color");
        glUniform3fv(glGetUniformLocation(program, emission_color), 1,
                     mat.emission_color.raw);

        char emission_strength[64];
        strcpy(emission_strength, identifier);
        strcat(emission_strength, "emission_strength");
        glUniform1f(glGetUniformLocation(program, emission_strength),
                    mat.emission_strength);

        char transparency[64];
        strcpy(transparency, identifier);
        strcat(transparency, "transparency");
        glUniform1f(glGetUniformLocation(program, transparency),
                    mat.transparency);

        char refractive_index[64];
        strcpy(refractive_index, identifier);
        strcat(refractive_index, "refractive_index");
        glUniform1f(glGetUniformLocation(program, refractive_index),
                    mat.refractive_index);
    }
}

void uniform_objects(scene *world, int program) {
    glUseProgram(program);

    int spheres = 0;
    int triangles = 0;
    for (int i = 0; i < world->num_objects; i++) {
        shape obj = world->objects[i];
        char identifier[32];

        switch (obj.tag) {
        case SPHERE: {
            sprintf(identifier, "spheres[%d].", spheres);

            char center[64];
            strcpy(center, identifier);
            strcat(center, "center");
            glUniform3fv(glGetUniformLocation(program, center), 1,
                         obj.sphere.center.raw);

            char radius[64];
            strcpy(radius, identifier);
            strcat(radius, "radius");
            glUniform1f(glGetUniformLocation(program, radius),
                        obj.sphere.radius);

            spheres++;
            break;
        }
        case TRIANGLE: {
            sprintf(identifier, "triangles[%d].", triangles);

            char v0[64];
            strcpy(v0, identifier);
            strcat(v0, "v0");
            glUniform3fv(glGetUniformLocation(program, v0), 1,
                         obj.triangle.v0.raw);

            char v1[64];
            strcpy(v1, identifier);
            strcat(v1, "v1");
            glUniform3fv(glGetUniformLocation(program, v1), 1,
                         obj.triangle.v1.raw);

            char v2[64];
            strcpy(v2, identifier);
            strcat(v2, "v2");
            glUniform3fv(glGetUniformLocation(program, v2), 1,
                         obj.triangle.v2.raw);

            triangles++;
            break;
        }
        }

        char material[64];
        strcpy(material, identifier);
        strcat(material, "material");
        glUniform1i(glGetUniformLocation(program, material), obj.material);
    }
    glUniform1i(glGetUniformLocation(program, "sphere_count"), spheres);
    glUniform1i(glGetUniformLocation(program, "triangle_count"), triangles);
}

int main() {
    // Seed RNG
    srandom(time(NULL));

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
    int random_seed_loc = glGetUniformLocation(shader, "random_seed");
    int window_size_loc = glGetUniformLocation(shader, "window_size");
    int aspect_ratio_loc = glGetUniformLocation(shader, "aspect_ratio");
    int fov_loc = glGetUniformLocation(shader, "fov");
    int sky_color_loc = glGetUniformLocation(shader, "sky_color");

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

    scene world;
    scene_init(&world);

    /*world.sky_color = (vec3s){135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};*/
    world.sky_color = glms_vec3_broadcast(0.0);

    int sun =
        scene_add_material(&world, (vec3s){0.9372, 0.7490, 0.0157}, 0.3, 1.0,
                           (vec3s){0.9372, 0.7490, 0.0157}, 1.0, 0.0, 1.0);
    int red_plastic = scene_add_material(&world, (vec3s){1, 0, 0}, 0.5, 0.5,
                                         (vec3s){1, 0, 0}, 0.0, 0.0, 1.0);
    int green_grass = scene_add_material(
        &world, (vec3s){65.0 / 255.0, 152.0 / 255.0, 10.0 / 255.0}, 1.0, 0.1,
        (vec3s){65.0 / 255.0, 152.0 / 255.0, 10.0 / 255.0}, 0.1, 0.0, 1.0);
    int mirror = scene_add_material(&world, (vec3s){1, 1, 1}, 0.0, 1.0,
                                    glms_vec3_zero(), 0.0, 0.0, 1.0);
    int glass = scene_add_material(&world, (vec3s){1.0, 1.0, 1.0}, 0.0, 0.0,
                                   (vec3s){1.0, 1.0, 1.0}, 0.0, 1.0, 1.52);

    scene_add_sphere(&world, (vec3s){25.0, 25.0, 40}, 25, sun);
    scene_add_sphere(&world, (vec3s){-1.5, 0.0, 5.0}, 1, red_plastic);
    scene_add_sphere(&world, (vec3s){0.5, -0.5, 2.5}, 0.5, glass);
    /*scene_add_sphere(&world, (vec3s){0.0, -101.0, 5.0}, 100, green_grass);*/

    scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){50, -1, -50},
                       (vec3s){50, -1, 50}, green_grass);
    scene_add_triangle(&world, (vec3s){-50, -1, -50}, (vec3s){-50, -1, 50},
                       (vec3s){50, -1, 50}, green_grass);

    /*
    scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){-2.5, -2.5, 10},
                       (vec3s){2.5, -2.5, 10}, mirror);
    scene_add_triangle(&world, (vec3s){-2.5, 2.5, 10}, (vec3s){2.5, 2.5, 10},
                       (vec3s){2.5, -2.5, 10}, mirror);

    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){-5, -2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);
    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){5, 2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);
                       */

    glUseProgram(shader);

    // Send materials and objects to GPU
    uniform_materials(&world, shader);
    uniform_objects(&world, shader);

#ifdef RT

    // Real-time render loop
    double time = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double prevTime = time;
        time = glfwGetTime();
        double delta_time = time - prevTime;
        /*printf("%f\n", time);*/

        /*world.objects[1].sphere.center.x = 1.2 * cos(0.4 * time) - 1.0;*/
        /*world.objects[1].sphere.center.z = 1.2 * sin(0.4 * time) + 5.0;*/
        /*world.objects[1].sphere.radius += 0.1 * delta_time;*/
        uniform_objects(&world, shader);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 1.0);

        glUseProgram(shader);
        glBindVertexArray(vao);

        // Set general settings
        glUniform1ui(random_seed_loc, (unsigned int)random());
        glUniform2f(window_size_loc, WIDTH, HEIGHT);
        glUniform1f(aspect_ratio_loc, (float)width / (float)(height));
        glUniform1f(fov_loc, fov);
        glUniform3fv(sky_color_loc, 1, world.sky_color.raw);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#else

    // Create framebuffer to draw scene to
    GLuint fbo;
    GLuint texture;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer not complete!\n");
        return 1;
    }

    // Bind and clear the buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set general settings
    glUseProgram(shader);
    glBindVertexArray(vao);
    glUniform1ui(random_seed_loc, (unsigned int)random());
    glUniform2f(window_size_loc, WIDTH, HEIGHT);
    glUniform1f(aspect_ratio_loc, (float)WIDTH / (float)HEIGHT);
    glUniform1f(fov_loc, fov);
    glUniform3fv(sky_color_loc, 1, world.sky_color.raw);

    // Render scene
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Read and save rendered scene pixel data
    GLubyte pixels[3 * WIDTH * HEIGHT];
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    write_bitmap("output.bmp", WIDTH, HEIGHT, pixels, false);

    // Release resources
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

#endif

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}
