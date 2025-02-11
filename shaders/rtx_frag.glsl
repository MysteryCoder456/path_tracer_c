#version 410

#define M_PI 3.1415926535897932384626433832795
#define MAX_BOUNCES 3
#define NUM_SAMPLES 8

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

uniform float random_seed;

uniform float aspect_ratio;
uniform float fov;
uniform vec3 sky_color;
uniform Material materials[32];

uniform Sphere spheres[32];
uniform int sphere_count;

uniform Triangle triangles[32];
uniform int triangle_count;

int sample_id;

float tan_fov_2;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233)) + random_seed + sample_id) * 43758.5453);
}

vec3 rand_unit_sphere(vec3 seed) {
    return normalize(vec3(rand(seed.xy), rand(seed.yz), rand(seed.zx)));
}

float ray_sphere_intersect(vec3 o, vec3 d, Sphere s) {
    // Translate ray so that sphere is at origin
    vec3 o1 = o - s.center;

    // Quadratic equation
    float a = dot(d, d);
    float b = 2.0 * (o1.x * d.x + o1.y * d.y + o1.z * d.z);
    float c = dot(o1, o1) - pow(s.radius, 2.0);
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

vec3 incident_light(vec3 origin, vec3 direction) {
    vec3 color = vec3(1.0);
    vec3 total_light = vec3(0.0);

    int bounces = 0;
    while (true) {
        if (bounces > MAX_BOUNCES) {
            total_light += color * sky_color;
            break;
        }

        // Trace ray
        RayHit hit = trace_ray(origin, direction);
        if (hit.distance < 0) {
            total_light += color * sky_color;
        }
        Material mat = materials[hit.material];

        // Surface emission
        vec3 Le = mat.emission_color * mat.emission_strength;

        // Roughness normal
        vec3 deviation = rand_unit_sphere(origin + direction) * mat.roughness * 0.5;
        vec3 normal = normalize(hit.normal + deviation);

        // Adding contributions
        total_light += color * Le;
        color *= mat.albedo;

        // Prepare for next bounce
        vec3 reflect_direction = normalize(reflect(direction, normal));
        vec3 reflect_origin = hit.point + 0.0001 * reflect_direction;
        origin = reflect_origin;
        direction = reflect_direction;
        bounces++;
    }

    // TODO: Transmitted incident light

    // vec3 transmitted_Li = vec3(0.0);
    // if (mat.transparency > 0.0) {
    //     float dot = dot(direction, normal);
    //
    //     float refractive_index;
    //     vec3 refraction_normal;
    //     if (dot < 0) {
    //         refractive_index = 1.0 / 1.5;
    //         refraction_normal = normal;
    //     } else {
    //         refractive_index = 1.5;
    //         refraction_normal = -normal;
    //     }
    //
    //     vec3 transmit_direction = refract(direction, refraction_normal, refractive_index);
    //     vec3 transmit_origin = hit.point + 0.0001 * transmit_direction;
    //     transmitted_Li = incident_light(transmit_origin, transmit_direction, bounces);
    // }

    // return Le + mat.albedo * Li;
    return total_light;
}

vec3 per_pixel() {
    vec3 origin = vec3(0.0);
    vec3 direction = normalize(vec3(
                coords.x * tan_fov_2,
                coords.y * tan_fov_2 / aspect_ratio,
                1.0));

    // Multisampling
    vec3 result = vec3(0.0);
    for (sample_id = 0; sample_id < NUM_SAMPLES; sample_id++)
        result += incident_light(origin, direction);
    result /= NUM_SAMPLES;

    return result;
}

void main() {
    tan_fov_2 = tan(fov / 2.0);
    FragColor = vec4(per_pixel(), 1.0);
}
