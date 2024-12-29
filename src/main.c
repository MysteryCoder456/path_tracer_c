#include "bitmap.h"
#include "cglm/struct.h"
#include "shapes.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 1152
#define HEIGHT 720
#define FOV M_PI / 180.f * 90.0f
#define MAX_BOUNCES 2
#define NUM_SAMPLES 32

vec3s trace_ray(const vec3s initial_origin, const vec3s initial_direction,
                const vec3s light_dir, shape const *objects,
                const size_t num_objects) {
    size_t bounces;
    vec3s result = glms_vec3_zero();

    vec3s origin = initial_origin;
    vec3s dir = initial_direction;

    for (bounces = 1; bounces <= MAX_BOUNCES; bounces++) {
        float closest_dist = INFINITY;
        vec3s closest_point;
        vec3s closest_normal;
        shape_material closest_material;

        // Find closest intersection
        for (int i = 0; i < num_objects; i++) {
            shape obj = objects[i];

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
                // TODO: Implement triangles
                break;
            }
            }
        }

        if (closest_dist == INFINITY) {
            // TODO: sky color

            // sky blue
            vec3s sky_color = {135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0};
            /*vec3s sky_color = {0, 0, 0};*/

            result = glms_vec3_add(result, sky_color);
            break;
        }

        // Diffuse
        float diffuse_dot = fmaxf(-glms_vec3_dot(closest_normal, light_dir), 0);
        result = glms_vec3_add(
            result, glms_vec3_scale(closest_material.albedo, diffuse_dot));

        // Ray bounce
        vec3s random_vector = {(float)rand() / RAND_MAX,
                               (float)rand() / RAND_MAX,
                               (float)rand() / RAND_MAX};
        vec3s deviation = glms_vec3_normalize(
            glms_vec3_sub(glms_vec3_scale(random_vector, 2), glms_vec3_one()));
        vec3s deviated_normal =
            glms_vec3_normalize(glms_vec3_add(closest_normal, deviation));
        vec3s normal = glms_vec3_lerpc(closest_normal, deviated_normal,
                                       closest_material.roughness);

        // Prepare next bounce
        dir = glms_vec3_reflect(dir, normal);
        origin = glms_vec3_add(closest_point, glms_vec3_scale(dir, 0.001));
    }

    result = glms_vec3_scale(result, 1.0f / bounces);
    result = glms_vec3_clamp(result, 0, 1);
    return result;
}

int main() {
    // Seed rng
    srand(time(NULL));

    // Define the space
    vec3s light_dir = glms_vec3_normalize((vec3s){1.0, -0.8, 0.4});
    shape objects[] = {
        {.tag = SPHERE,
         .material =
             {
                 .albedo = {1, 0, 0},
                 .roughness = 0.9,
                 .metallicity = 1.0,
             },
         .sphere = {{0, 0, 5}, 1}},
        {.tag = SPHERE,
         .material =
             {
                 .albedo = {63.0 / 255.0, 155.0 / 255.0, 11 / 255.0},
                 .roughness = 0.1,
                 .metallicity = 1.0,
             },
         .sphere = {{0, -21, 5}, 20}},
    };
    size_t num_objects = 2;

    // Initialize frame and fill it with black
    uint8_t frame[WIDTH * HEIGHT][3];
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        frame[i][0] = 0;
        frame[i][1] = 0;
        frame[i][2] = 0;
    }

    // Render the scene
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        int frame_x = i % WIDTH;
        int frame_y = i / WIDTH;

        // Calculate ray direction based on FOV and frame dimensions
        float tangent = tanf(FOV / 2.0f);
        float x = ((float)frame_x / WIDTH * 2.0f - 1.0f) * tangent;
        float y =
            -((float)frame_y / HEIGHT * 2.0f - 1.0f) * tangent * HEIGHT / WIDTH;

        vec3s initial_origin = {0, 0, 0};
        vec3s initial_direction = glms_vec3_normalize((vec3s){x, y, 1});

        // Multisampling
        vec3s result = glms_vec3_zero();
        for (int j = 0; j < NUM_SAMPLES; j++) {
            // Trace the ray
            vec3s sample = trace_ray(initial_origin, initial_direction,
                                     light_dir, objects, num_objects);
            result = glms_vec3_add(result, sample);
        }
        result = glms_vec3_scale(result, 1.0f / NUM_SAMPLES);

        // Convert to 0-255 range
        result = glms_vec3_scale(result, 255);

        frame[i][0] = result.r;
        frame[i][1] = result.g;
        frame[i][2] = result.b;
    }

    // Write rendered scene to an image
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);
    return 0;
}
