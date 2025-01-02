#include "ray.h"
#include "cglm/struct.h"

ray_hit trace_ray(const vec3s origin, const vec3s direction,
                  const scene *world) {
    float closest_dist = -1;
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
            if (!glms_ray_sphere(origin, direction, sphere, &t1, &t2))
                break;

            float dist = t1 > 0 ? t1 : t2;
            if (dist >= closest_dist && closest_dist > 0)
                break;

            vec3s intersect =
                glms_vec3_add(glms_vec3_scale(direction, dist), origin);
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
            if (!glms_ray_triangle(origin, direction, obj.triangle.v0,
                                   obj.triangle.v1, obj.triangle.v2, &dist))
                break;
            if (dist >= closest_dist && closest_dist > 0)
                break;

            vec3s intersect =
                glms_vec3_add(glms_vec3_scale(direction, dist), origin);
            vec3s normal = glms_vec3_crossn(
                glms_vec3_sub(obj.triangle.v0, obj.triangle.v1),
                glms_vec3_sub(obj.triangle.v0, obj.triangle.v2));

            // Check if normal is facing the right way
            if (glms_vec3_dot(normal, direction) > 0)
                normal = glms_vec3_negate(normal);

            closest_dist = dist;
            closest_point = intersect;
            closest_normal = normal;
            closest_material = obj.material;
            break;
        }
        }
    }

    return (ray_hit){
        .distance = closest_dist,
        .point = closest_point,
        .normal = closest_normal,
        .material = closest_material,
    };
}
