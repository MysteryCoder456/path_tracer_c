#include "renderer.h"
#include "cglm/struct.h"
#include "ray.h"
#include <math.h>

#define RANDOM_MAX 0x7FFFFFFF
#define FOV M_PI / 180.f * 90.0f
#define MAX_BOUNCES 4
#define NUM_SAMPLES 16

vec3s rand_unit_sphere() {
    vec3s random_vector = {(double)random() / (double)RANDOM_MAX,
                           (double)random() / (double)RANDOM_MAX,
                           (double)random() / (double)RANDOM_MAX};
    vec3s unit_range =
        glms_vec3_sub(glms_vec3_scale(random_vector, 2), glms_vec3_one());
    vec3s unit_sphere = glms_vec3_normalize(unit_range);
    return unit_sphere;
}

vec3s incident_light(vec3s origin, vec3s direction, const scene *world,
                     size_t bounces) {
    // Recursion base case
    if (bounces > MAX_BOUNCES)
        return world->sky_color;
    bounces++;

    ray_hit hit = trace_ray(origin, direction, world);

    // Ray didn't hit anything
    if (hit.distance < 0)
        return world->sky_color;

    shape_material mat = world->materials[hit.material];

    // Surface emission
    vec3s Le = glms_vec3_scale(mat.emission_color, mat.emission_strength);

    // Normal based on roughness
    vec3s deviation = glms_vec3_scale(rand_unit_sphere(), mat.roughness * 0.5);
    vec3s normal = glms_vec3_normalize(glms_vec3_add(hit.normal, deviation));

    // Calculate reflected incident light
    vec3s reflected_Li = glms_vec3_zero();
    if (mat.transparency < 1.0) {
        vec3s reflect_direction =
            glms_vec3_normalize(glms_vec3_reflect(direction, normal));
        vec3s reflect_origin =
            glms_vec3_add(hit.point, glms_vec3_scale(reflect_direction, 0.001));
        reflected_Li =
            incident_light(reflect_origin, reflect_direction, world, bounces);
    }

    // Calculate transmitted incident light
    vec3s transmitted_Li = glms_vec3_zero();
    if (mat.transparency > 0.0) {
        float dot = glms_vec3_dot(direction, normal);
        float refractive_index = dot < 0 ? 1.5 : 1 / 1.5;
        vec3s refraction_normal = dot < 0 ? normal : glms_vec3_negate(normal);

        vec3s transmit_origin, transmit_direction;
        if (glms_vec3_refract(direction, refraction_normal,
                              1.0 / refractive_index, &transmit_direction)) {
            transmit_direction = glms_vec3_normalize(transmit_direction);
            transmit_origin = glms_vec3_add(
                hit.point, glms_vec3_scale(transmit_direction, 0.001));
            transmitted_Li = incident_light(transmit_origin, transmit_direction,
                                            world, bounces);
        }
    }

    vec3s Li = glms_vec3_lerp(reflected_Li, transmitted_Li, mat.transparency);
    return glms_vec3_add(Le, glms_vec3_mul(Li, mat.albedo));
}

vec3s per_pixel(const float x, const float y, const float aspect_ratio,
                const scene *world) {
    const float tangent_fov_2 = tanf(FOV / 2.0f);

    vec3s origin = {0, 0, 0};
    vec3s direction = glms_vec3_normalize((vec3s){
        x * tangent_fov_2,
        y * tangent_fov_2 / aspect_ratio,
        1,
    });

    vec3s result = incident_light(origin, direction, world, 0);
    result = glms_vec3_clamp(result, 0, 1);
    return result;
}

void render_task(render_args *args) {
    render_args copied_args = *args;
    float aspect_ratio = (float)copied_args.framew / (float)copied_args.frameh;

    for (int screen_x = 0; screen_x < copied_args.framew; screen_x++) {
        // Calculate screen-space coordinates
        float x = ((float)screen_x / copied_args.framew * 2.0f - 1.0f);
        float y =
            -((float)copied_args.screen_y / copied_args.frameh * 2.0f - 1.0f);

        // Calculate pixel color
        vec3s result = glms_vec3_zero();
        for (int j = 0; j < NUM_SAMPLES; j++) {
            vec3s sample = per_pixel(x, y, aspect_ratio, copied_args.world);
            result = glms_vec3_add(result, sample);
        }
        result = glms_vec3_scale(result, 1.0f / NUM_SAMPLES * 255.0f);

        // Write to frame buffer
        int idx = copied_args.screen_y * copied_args.framew + screen_x;
        copied_args.frame[3 * idx + 0] = result.r;
        copied_args.frame[3 * idx + 1] = result.g;
        copied_args.frame[3 * idx + 2] = result.b;
    }
}
