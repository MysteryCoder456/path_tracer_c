#include "bitmap.h"
#include "cglm/struct.h"
#include "ray.h"
#include "scene.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 1280
#define HEIGHT 800
#define FOV M_PI / 180.f * 90.0f
#define MAX_BOUNCES 4
#define NUM_SAMPLES 4

vec3s per_pixel(const float x, const float y, const scene *world) {
    const float tangent_fov_2 = tanf(FOV / 2.0f);
    const float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    vec3s origin = {0, 0, 0};
    vec3s direction = glms_vec3_normalize((vec3s){
        x * tangent_fov_2,
        y * tangent_fov_2 / aspect_ratio,
        1,
    });

    size_t bounces;
    float multiplier = 1.0f;
    vec3s result = glms_vec3_zero();

    for (bounces = 1; bounces <= MAX_BOUNCES; bounces++) {
        ray_hit hit = trace_ray(origin, direction, world);

        // Ray didn't hit anything
        if (hit.distance < 0) {
            result = glms_vec3_add(
                result, glms_vec3_scale(world->sky_color, multiplier));
            break;
        }

        // Diffuse
        float diffuse_dot =
            fmaxf(-glms_vec3_dot(hit.normal, world->light_dir), 0);
        result = glms_vec3_add(
            result, glms_vec3_scale(world->materials[hit.material].albedo,
                                    multiplier * diffuse_dot));

        // Ray bounce
        vec3s random_vector = {(float)rand() / RAND_MAX,
                               (float)rand() / RAND_MAX,
                               (float)rand() / RAND_MAX};
        vec3s deviation = glms_vec3_scale(
            glms_vec3_normalize(glms_vec3_sub(glms_vec3_scale(random_vector, 2),
                                              glms_vec3_one())),
            0.5);
        vec3s deviated_normal =
            glms_vec3_normalize(glms_vec3_add(hit.normal, deviation));
        vec3s normal =
            glms_vec3_lerpc(hit.normal, deviated_normal,
                            world->materials[hit.material].roughness);

        // Prepare next ray bounce
        direction = glms_vec3_reflect(direction, normal);
        origin = glms_vec3_add(hit.point, glms_vec3_scale(direction, 0.001));
        multiplier *= world->materials[hit.material].metallicity;
    }

    result = glms_vec3_scale(result, 1.0f / bounces);
    result = glms_vec3_clamp(result, 0, 1);
    return result;
}

int main() {
    // Seed rng
    srand(time(NULL));

    // Define the scene
    scene world;
    scene_init(&world);

    world.light_dir = glms_vec3_normalize((vec3s){0.0, -1.0, 0.0});
    world.light_color = (vec3s){1, 1, 1};
    world.sky_color = (vec3s){135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};

    int red_metal =
        scene_add_material(&world, (vec3s){0.8, 0.1, 0.1}, 0.2, 0.8);
    int green_grass =
        scene_add_material(&world, (vec3s){0.2471, 0.6078, 0.0431}, 0.6, 0.2);
    int mirror = scene_add_material(&world, (vec3s){0, 0, 0}, 0.0, 1.0);

    scene_add_sphere(&world, (vec3s){2.5, 0, 5}, 1, red_metal);
    scene_add_sphere(&world, (vec3s){0, -21, 5}, 20, green_grass);
    scene_add_triangle(&world, (vec3s){-5, 2.5, 10}, (vec3s){-5, -2.5, 10},
                       (vec3s){5, -2.5, 10}, mirror);
    scene_add_triangle(&world, (vec3s){-5, 2.5, 10}, (vec3s){5, 2.5, 10},
                       (vec3s){5, -2.5, 10}, mirror);

    // Initialize frame and fill it with black
    uint8_t frame[WIDTH * HEIGHT][3];
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        frame[i][0] = 0;
        frame[i][1] = 0;
        frame[i][2] = 0;
    }

    // Render the scene
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        // Calculate screen-space coordinates
        int x = i % WIDTH;
        int y = i / WIDTH;
        float screen_x = ((float)x / WIDTH * 2.0f - 1.0f);
        float screen_y = -((float)y / HEIGHT * 2.0f - 1.0f);

        // Calculate pixel color
        vec3s result = glms_vec3_zero();
        for (int j = 0; j < NUM_SAMPLES; j++) {
            vec3s sample = per_pixel(screen_x, screen_y, &world);
            result = glms_vec3_add(result, sample);
        }
        result = glms_vec3_scale(result, 1.0f / NUM_SAMPLES);

        // Convert to 0-255 range and write to frame buffer
        result = glms_vec3_scale(result, 255);
        frame[i][0] = result.r;
        frame[i][1] = result.g;
        frame[i][2] = result.b;
    }

    // Write rendered scene to an image then cleanup
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);
    scene_free(&world);
    return 0;
}
