#version 410

#define M_PI 3.1415926535897932384626433832795
#define MAX_BOUNCES 4

struct Material {
    vec3 albedo;
    float roughness;
    float metallicity;

    vec3 emission_color;
    float emission_strength;

    float transparency;
};

struct RayHit {
    float distance;
    vec3 point;
    vec3 normal;
    int material;
};

struct Sphere {
    vec3 center;
    float radius;
    int material;
};
struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
    int material;
};

in vec2 coords;
out vec4 FragColor;

uniform float aspect_ratio;
uniform float fov;
uniform vec3 sky_color;
uniform Material materials[32];

uniform Sphere spheres[32];
uniform int sphere_count;

uniform Triangle triangles[32];
uniform int triangle_count;

float tan_fov_2;

float ray_sphere_intersect(vec3 o, vec3 d, Sphere s) {
    // Translate ray so that sphere is at origin
    o -= s.center;

    // Quadratic equation
    float a = dot(d, d);
    float b = 2.0 * (o.x * d.x + o.y * d.y + o.z * d.z);
    float c = dot(o, o) - pow(s.radius, 2.0);
    float det = b * b - 4.0 * a * c;

    // Equation has no real solutions
    if (det < 0)
        return -1;

    float t1 = (-b - sqrt(det)) / 2.0 * a;
    if (t1 >= 0)
        return t1;

    float t2 = (-b + sqrt(det)) / 2.0 * a;
    if (t2 >= 0)
        return t2;

    return -1;
}

// Moller-Trumbore algorithm
// Adapted from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_implementation
float ray_triangle_intersect(vec3 o, vec3 d, Triangle tr) {
    const float epsilon = 1e-6;

    vec3 edge1 = tr.v1 - tr.v0;
    vec3 edge2 = tr.v2 - tr.v0;
    vec3 ray_cross_e2 = cross(d, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det > -epsilon && det < epsilon)
        return -1;

    float inv_det = 1.0 / det;
    vec3 s = o - tr.v0;
    float u = inv_det * dot(s, ray_cross_e2);

    if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
        return -1;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(d, s_cross_e1);

    if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
        return -1;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);
    return t;
}

RayHit trace_ray(vec3 origin, vec3 direction) {
    RayHit closest_hit;
    closest_hit.distance = -1;

    // Check spheres
    for (int i = 0; i < sphere_count; i++) {
        float dist = ray_sphere_intersect(origin, direction, spheres[i]);

        if (closest_hit.distance > 0 && (dist < 0 || dist >= closest_hit.distance))
            continue;

        closest_hit.distance = dist;
        closest_hit.point = origin + dist * direction;
        closest_hit.normal = normalize(closest_hit.point - spheres[i].center);
        closest_hit.material = spheres[i].material;
    }

    // Check triangles
    for (int i = 0; i < triangle_count; i++) {
        float dist = ray_triangle_intersect(origin, direction, triangles[i]);

        if (closest_hit.distance > 0 && (dist < 0 || dist >= closest_hit.distance))
            continue;

        closest_hit.distance = dist;
        closest_hit.point = origin + dist * direction;

        vec3 normal = normalize(cross(triangles[i].v0 - triangles[i].v1, triangles[i].v0 - triangles[i].v2));
        closest_hit.normal = (2.0 * int(dot(direction, normal) < 0) - 1.0) * normal;

        closest_hit.material = spheres[i].material;
    }

    return closest_hit;
}

vec3 incident_light(vec3 origin, vec3 direction, int bounces) {
    // Recursion base case
    if (bounces > MAX_BOUNCES)
        return sky_color;
    bounces++;

    // Trace ray
    RayHit hit = trace_ray(origin, direction);
    if (hit.distance < 0)
        return sky_color;
    Material mat = materials[hit.material];

    // Surface emission
    vec3 Le = mat.emission_color * mat.emission_strength;

    // Roughness normal
    vec3 deviation = normalize(noise3(hit.point.x + hit.point.y + hit.point.z)) * mat.roughness * 0.5;
    vec3 normal = normalize(hit.normal + deviation);

    // Reflected incident light
    vec3 reflected_Li = vec3(0.0);
    if (mat.transparency < 1.0) {
        vec3 reflect_direction = normalize(reflect(direction, normal));
        vec3 reflect_origin = hit.point + 0.0001 * reflect_direction;
        reflected_Li = incident_light(reflect_origin, reflect_direction, bounces);
    }

    // Transmitted incident light
    vec3 transmitted_Li = vec3(0.0);
    if (mat.transparency > 0.0) {
        float dot = dot(direction, normal);

        float refractive_index;
        vec3 refraction_normal;
        if (dot < 0) {
            refractive_index = 1.0 / 1.5;
            refraction_normal = normal;
        } else {
            refractive_index = 1.5;
            refraction_normal = -normal;
        }

        vec3 transmit_direction = refract(direction, refraction_normal, refractive_index);
        vec3 transmit_origin = hit.point + 0.0001 * transmit_direction;
        transmitted_Li = incident_light(transmit_origin, transmit_direction, bounces);
    }

    vec3 Li = mix(reflected_Li, transmitted_Li, mat.transparency);
    return Le + mat.albedo * Li;
}

vec3 per_pixel() {
    vec3 origin = vec3(0.0);
    vec3 direction = vec3(
            coords.x * tan_fov_2,
            coords.y * tan_fov_2 / aspect_ratio,
            1.0);

    return incident_light(origin, direction, 0);
}

void main() {
    tan_fov_2 = tan(fov / 2.0);
    FragColor = vec4(per_pixel(), 1.0);
}
