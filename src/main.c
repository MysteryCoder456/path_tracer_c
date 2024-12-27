#include "bitmap.h"
#include "cglm/struct.h"
#include "shapes.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 720
#define HEIGHT 720
#define FOV (M_PI / 180.0f * 70.0f)

vec3s trace_ray(vec3s origin, vec3s dir, shape const *objects,
                const size_t num_objects) {
    float closest_dist = INFINITY;
    vec3s closest_point;
    vec3s closest_normal;
    vec3s closest_color = {0, 0, 0};

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

            float dist = t1 > 0 ? t1 : (t2 > 0 ? t2 : 0);
            if (dist >= closest_dist)
                break;

            vec3s intersect = glms_vec3_add(glms_vec3_scale(dir, dist), origin);
            vec3s normal = glms_vec3_sub(intersect, obj.sphere.center);
            glms_vec3_normalize(normal);

            closest_dist = dist;
            closest_point = intersect;
            closest_normal = normal;
            closest_color = obj.color;
            break;
        }
        case TRIANGLE: {
            // TODO: Implement triangles
            break;
        }
        }
    }

    return closest_color;
}

int main() {
    // Create some objects in space
    shape objects[] = {
        {SPHERE, {0.9, 0.1, 0.1}, .sphere = {{2, 0, 5}, 1}},
        {SPHERE, {0.5, 0.7, 0.2}, .sphere = {{0, 0, 4}, 1}},
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
        float y = (frame_y - (float)HEIGHT / 2) / ((float)HEIGHT / 2) * tangent;

        vec3s ray_ori = {0, 0, 0};
        vec3s ray_dir = {x, y, 1};
        glms_vec3_normalize(ray_dir);

        vec3s result = trace_ray(ray_ori, ray_dir, objects, num_objects);

        // convert 0-1 range to 0-255 range
        result = glms_vec3_scale(result, 255);

        frame[i][0] = result.r;
        frame[i][1] = result.g;
        frame[i][2] = result.b;
    }

    // Write rendered scene to an image
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);
    return 0;
}
