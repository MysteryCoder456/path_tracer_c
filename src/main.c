#include "bitmap.h"
#include "cglm/struct.h"
#include "shapes.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 720
#define HEIGHT 720
#define FOV (M_PI / 180.0f * 70.0f)

vec3s trace_ray(vec3s origin, vec3s dir, const vec3s light_dir,
                shape const *objects, const size_t num_objects) {
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

            vec3s intersect = glms_vec3_add(glms_vec3_scale(dir, dist), origin);
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

    if (closest_dist == INFINITY)
        // TODO: sky color
        return glms_vec3_zero();

    vec3s result = glms_vec3_zero();

    // Ambient
    result = glms_vec3_add(result, closest_material.ambient);

    // Diffuse
    float diffuse_dot = fmaxf(-glms_vec3_dot(closest_normal, light_dir), 0);
    result = glms_vec3_add(
        result, glms_vec3_scale(closest_material.diffuse, diffuse_dot));

    // Specular
    vec3s reflect = glms_vec3_normalize(glms_vec3_reflect(dir, closest_normal));
    float specular_dot = fmaxf(-glms_vec3_dot(reflect, light_dir), 0);
    result = glms_vec3_add(
        result,
        glms_vec3_scale(closest_material.specular,
                        powf(specular_dot, closest_material.shininess)));

    return result;
}

int main() {
    // Define the space
    vec3s light_dir = glms_vec3_normalize((vec3s){1, -1, 0.5});
    shape objects[] = {
        {.tag = SPHERE,
         .material = {.ambient = {0.1745, 0.01175, 0.01175},
                      .diffuse = {0.61424, 0.04136, 0.04136},
                      .specular = {0.727811, 0.626959, 0.626959},
                      .shininess = 128.0},
         .sphere = {{0, 0, 3}, 1}},
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
        float tangent = tan(FOV / 2);
        float x = (frame_x - (float)WIDTH / 2) / ((float)WIDTH / 2) * tangent;
        float y =
            -(frame_y - (float)HEIGHT / 2) / ((float)HEIGHT / 2) * tangent;

        vec3s ray_ori = {0, 0, 0};
        vec3s ray_dir = {x, y, 1};
        glms_vec3_normalize(ray_dir);

        vec3s result =
            trace_ray(ray_ori, ray_dir, light_dir, objects, num_objects);

        // Clamp components b/w 0 and 1 then convert to 0-255 range
        result = glms_vec3_clamp(result, 0, 1);
        result = glms_vec3_scale(result, 255);

        frame[i][0] = result.r;
        frame[i][1] = result.g;
        frame[i][2] = result.b;
    }

    // Write rendered scene to an image
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);
    return 0;
}
