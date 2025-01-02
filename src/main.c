#include "bitmap.h"
#include "cglm/struct.h"
#include "scene.h"
#include "shapes.h"
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
    vec3s dir = glms_vec3_normalize((vec3s){
        x * tangent_fov_2,
        y * tangent_fov_2 / aspect_ratio,
        1,
    });

    size_t bounces;
    float multiplier = 1.0f;
    vec3s result = glms_vec3_zero();

    for (bounces = 1; bounces <= MAX_BOUNCES; bounces++) {
        float closest_dist = INFINITY;
        vec3s closest_point;
        vec3s closest_normal;
        size_t closest_material;

        // Find closest intersection
        for (int i = 0; i < world->num_objects; i++) {
            shape obj = world->objects[i];

            switch (obj.tag) {
            case SPHERE: {
                // Check whether ray intersects sphere
                vec4s sphere = {obj.sphere.center.x, obj.sphere.center.y,
                                obj.sphere.center.z, obj.sphere.radius};
                float t1, t2;
                if (!glms_ray_sphere(origin, dir, sphere, &t1, &t2))
                    break;

                float dist = t1 > 0 ? t1 : t2;
                if (dist >= closest_dist)
                    break;

                vec3s intersect =
                    glms_vec3_add(glms_vec3_scale(dir, dist), origin);
                vec3s normal = glms_vec3_normalize(
                    glms_vec3_sub(intersect, obj.sphere.center));

                closest_dist = dist;
                closest_point = intersect;
                closest_normal = normal;
                closest_material = obj.material;
                break;
            }
            case TRIANGLE: {
                // Check whether ray intersects triangle
                float dist;
                if (!glms_ray_triangle(origin, dir, obj.triangle.v0,
                                       obj.triangle.v1, obj.triangle.v2, &dist))
                    break;
                if (dist >= closest_dist)
                    break;

                vec3s intersect =
                    glms_vec3_add(glms_vec3_scale(dir, dist), origin);
                vec3s normal = glms_vec3_crossn(
                    glms_vec3_sub(obj.triangle.v0, obj.triangle.v1),
                    glms_vec3_sub(obj.triangle.v0, obj.triangle.v2));

                // Check if normal is facing the right way
                if (glms_vec3_dot(normal, dir) > 0)
                    normal = glms_vec3_negate(normal);

                closest_dist = dist;
                closest_point = intersect;
                closest_normal = normal;
                closest_material = obj.material;
                break;
            }
            }
        }

        if (closest_dist == INFINITY) {
            result = glms_vec3_add(
                result, glms_vec3_scale(world->sky_color, multiplier));
            break;
        }

        // Diffuse
        float diffuse_dot =
            fmaxf(-glms_vec3_dot(closest_normal, world->light_dir), 0);
        result = glms_vec3_add(
            result, glms_vec3_scale(world->materials[closest_material].albedo,
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
            glms_vec3_normalize(glms_vec3_add(closest_normal, deviation));
        vec3s normal =
            glms_vec3_lerpc(closest_normal, deviated_normal,
                            world->materials[closest_material].roughness);

        // Prepare next ray bounce
        dir = glms_vec3_reflect(dir, normal);
        origin = glms_vec3_add(closest_point, glms_vec3_scale(dir, 0.001));
        multiplier *= world->materials[closest_material].metallicity;
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
