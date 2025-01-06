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
#define MAX_BOUNCES 8
#define NUM_SAMPLES 4

#define RANDOM_MAX 0x7FFFFFFF

vec3s rand_unit_sphere() {
    vec3s random_vector = {(double)random() / (double)RANDOM_MAX,
                           (double)random() / (double)RANDOM_MAX,
                           (double)random() / (double)RANDOM_MAX};
    vec3s unit_range =
        glms_vec3_sub(glms_vec3_scale(random_vector, 2), glms_vec3_one());
    vec3s unit_sphere = glms_vec3_normalize(unit_range);
    return unit_sphere;
}

vec3s per_pixel(const float x, const float y, const scene *world) {
    const float tangent_fov_2 = tanf(FOV / 2.0f);
    const float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    vec3s origin = {0, 0, 0};
    vec3s direction = glms_vec3_normalize((vec3s){
        x * tangent_fov_2,
        y * tangent_fov_2 / aspect_ratio,
        1,
    });
    vec3s color = glms_vec3_one();
    float color_multiplier = 1.0;
    vec3s light = glms_vec3_zero();

    for (size_t bounces = 1; bounces <= MAX_BOUNCES; bounces++) {
        ray_hit hit = trace_ray(origin, direction, world);

        // Ray didn't hit anything
        if (hit.distance < 0) {
            /*color = glms_vec3_mul(color, world->sky_color);*/
            light =
                glms_vec3_add(light, glms_vec3_mul(world->sky_color, color));
            break;
        }

        shape_material mat = world->materials[hit.material];

        // TODO: Global illumination

        // Add surface emission to ray's light
        light = glms_vec3_add(
            light, glms_vec3_scale(mat.emission_color, mat.emission_strength));

        // Impart surface albedo onto ray's color
        vec3s mixed = glms_vec3_mul(color, mat.albedo);
        color = glms_vec3_lerpc(color, mixed, color_multiplier);
        color_multiplier *= mat.metallicity;

        // Calculate normal based on roughness
        vec3s deviation = glms_vec3_scale(rand_unit_sphere(), 0.5);
        vec3s deviated_normal =
            glms_vec3_normalize(glms_vec3_add(hit.normal, deviation));
        vec3s normal = glms_vec3_normalize(
            glms_vec3_lerpc(hit.normal, deviated_normal, mat.roughness));

        // Prepare next ray bounce
        direction = glms_vec3_reflect(direction, normal);
        origin = glms_vec3_add(hit.point, glms_vec3_scale(direction, 0.001));
    }

    // Combine light and color to produce final result
    /*vec3s result = glms_vec3_add(light, color);*/
    vec3s result = glms_vec3_clamp(light, 0, 1);
    return result;
}

int main() {
    // Seed rng
    srandom(time(NULL));

    // Define the scene
    scene world;
    scene_init(&world);

    world.light_direction = glms_vec3_normalize((vec3s){1.0, -1.0, 0.2});
    world.light_color = (vec3s){1, 1, 1};
    world.sky_color = (vec3s){135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};
    /*world.sky_color = glms_vec3_zero();*/

    int gold = scene_add_material(&world, (vec3s){0.9372, 0.7490, 0.0157}, 0.25,
                                  0.9, (vec3s){0.9372, 0.7490, 0.0157}, 1.0);
    int green_grass = scene_add_material(&world, (vec3s){0.0, 1.0, 0.0}, 0.6,
                                         0.2, (vec3s){0.0, 1.0, 0.0}, 0.0);
    int mirror = scene_add_material(&world, (vec3s){1, 1, 1}, 0.0, 1.0,
                                    glms_vec3_zero(), 0.0);
    int red_mirror = scene_add_material(&world, (vec3s){1, 0, 0}, 0.0, 1.0,
                                        (vec3s){1, 0, 0}, 0.0);

    scene_add_sphere(&world, (vec3s){2.5, 0.3, 5}, 1, gold);
    scene_add_sphere(&world, (vec3s){-2.5, 0.3, 5}, 1, red_mirror);
    scene_add_sphere(&world, (vec3s){0, -21, 5}, 20, green_grass);

    scene_add_triangle(&world, (vec3s){-5, 2.5, 10}, (vec3s){-5, -2.5, 10},
                       (vec3s){5, -2.5, 10}, mirror);
    scene_add_triangle(&world, (vec3s){-5, 2.5, 10}, (vec3s){5, 2.5, 10},
                       (vec3s){5, -2.5, 10}, mirror);

    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){-5, -2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);
    scene_add_triangle(&world, (vec3s){-5, 2.5, -10}, (vec3s){5, 2.5, -10},
                       (vec3s){5, -2.5, -10}, mirror);

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
