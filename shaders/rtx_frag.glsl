#version 410

#define M_PI 3.1415926535897932384626433832795
#define MAX_BOUNCES 4
#define NUM_SAMPLES 2048

struct Material {
    vec3 albedo;
    float roughness;
    float metallicity;

    vec3 emission_color;
    float emission_strength;

    float transparency;
    float refractive_index;
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

struct StackItem {
    vec3 origin;
    vec3 direction;
    int bounces;
    vec3 color;
};

in vec2 coords;
out vec4 FragColor;

uniform uint random_seed;

uniform vec2 window_size;
uniform float aspect_ratio;
uniform float fov;
uniform vec3 sky_color;
uniform Material materials[32];

uniform Sphere spheres[32];
uniform int sphere_count;

uniform Triangle triangles[32];
uniform int triangle_count;

float tan_fov_2;

// PCG (permuted congruential generator). Thanks to:
// www.pcg-random.org and www.shadertoy.com/view/XlGcRh
uint next_random(inout uint state) {
    state = state * 747796405u + 2891336453u + random_seed;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return result;
}

float random_value(inout uint state) {
    return next_random(state) / 4294967295.0; // 2^32 - 1
}

// Random value in normal distribution (with mean=0 and sd=1)
float random_value_normal_dist(inout uint state) {
    // Thanks to https://stackoverflow.com/a/6178290
    float theta = 2 * M_PI * random_value(state);
    float rho = sqrt(-2 * log(random_value(state)));
    return rho * cos(theta);
}

vec3 rand_unit_sphere(inout uint state) {
    float x = random_value_normal_dist(state);
    float y = random_value_normal_dist(state);
    float z = random_value_normal_dist(state);
    return normalize(vec3(x, y, z));
}

float ray_sphere_intersect(vec3 o, vec3 d, Sphere s) {
    // Translate ray so that sphere is at origin
    o -= s.center;

    // Quadratic equation
    float a = dot(d, d);
    float b = 2.0 * dot(o, d);
    float c = dot(o, o) - pow(s.radius, 2.0);
    float det = b * b - 4.0 * a * c;

    // Equation has no real solutions
    if (det < 0)
        return -1;
    float sqrt_det = sqrt(det);
    float two_a = 2.0 * a;

    float t1 = (-b - sqrt_det) / two_a;
    if (t1 >= 0)
        return t1;

    float t2 = (-b + sqrt_det) / two_a;
    if (t2 >= 0)
        return t2;

    return -1.0;
}

// Moller-Trumbore algorithm
// Adapted from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_implementation
float ray_triangle_intersect(vec3 o, vec3 d, Triangle tr) {
    const float epsilon = 1e-6;

    vec3 edge1 = tr.v1 - tr.v0;
    vec3 edge2 = tr.v2 - tr.v0;
    vec3 ray_cross_e2 = cross(d, edge2);
    float det = dot(edge1, ray_cross_e2);

    // Check if the ray is parallel to the triangle
    if (abs(det) < epsilon)
        return -1.0;

    float inv_det = 1.0 / det;
    vec3 s = o - tr.v0;
    float u = inv_det * dot(s, ray_cross_e2);

    // Check if intersection is outside the triangle
    if (u < epsilon || u > 1.0)
        return -1.0;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(d, s_cross_e1);

    // Check if intersection is outside the triangle
    if (v < epsilon || (u + v) > 1.0)
        return -1.0;

    // Compute t to determine intersection distance
    float t = inv_det * dot(edge2, s_cross_e1);

    // Check if intersection is behind the ray origin
    if (t < epsilon)
        return -1.0;

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

        closest_hit.material = triangles[i].material;
    }

    return closest_hit;
}

vec3 incident_light(vec3 origin, vec3 direction, inout uint rng_state) {
    // Create ray stack
    StackItem stack[(1 << MAX_BOUNCES) + 1];
    int stack_size = 0;

    // Add initial origin and direction to stack
    stack[stack_size].origin = origin;
    stack[stack_size].direction = direction;
    stack[stack_size].bounces = 0;
    stack[stack_size].color = vec3(1.0);
    stack_size++;

    vec3 total_light = vec3(0.0);

    while (stack_size > 0) {
        // Pop ray from stack
        StackItem current = stack[--stack_size];

        if (current.bounces > MAX_BOUNCES) {
            total_light += current.color * sky_color;
            continue;
        }

        // Trace ray
        RayHit hit = trace_ray(current.origin, current.direction);
        if (hit.distance < 0) {
            total_light += current.color * sky_color;
            continue;
        }
        Material mat = materials[hit.material];

        // Surface emission
        vec3 Le = mat.emission_color * mat.emission_strength;

        // Adding contributions
        total_light += current.color * Le;
        current.color *= mat.albedo;

        // Roughness normal
        vec3 deviation = rand_unit_sphere(rng_state) * mat.roughness;
        vec3 normal = normalize(hit.normal + deviation);

        // Prepare ray for reflection
        if (mat.transparency < 1.0) {
            vec3 reflect_direction = reflect(current.direction, normal);
            vec3 reflect_origin = hit.point + 0.0001 * reflect_direction;
            StackItem next_reflect;
            next_reflect.origin = reflect_origin;
            next_reflect.direction = reflect_direction;
            next_reflect.bounces = current.bounces + 1;
            next_reflect.color = (1 - mat.transparency) * current.color;
            stack[stack_size++] = next_reflect;
        }

        // Prepare ray for refraction
        if (mat.transparency > 0.0) {
            float dot = dot(current.direction, normal);
            float refractive_index;
            vec3 refraction_normal;
            if (dot < 0) {
                refractive_index = 1.0 / mat.refractive_index;
                refraction_normal = normal;
            } else {
                refractive_index = mat.refractive_index;
                refraction_normal = -normal;
            }
            vec3 transmit_direction = refract(current.direction, refraction_normal, refractive_index);
            vec3 transmit_origin = hit.point + 0.0001 * transmit_direction;
            StackItem next_refract;
            next_refract.origin = transmit_origin;
            next_refract.direction = transmit_direction;
            next_refract.bounces = current.bounces + 1;
            next_refract.color = mat.transparency * current.color;
            stack[stack_size++] = next_refract;
        }
    }

    return total_light;
}

vec3 per_pixel() {
    uint pixel_idx = uint((coords.y * window_size.y) * window_size.x + (coords.x * window_size.x));
    uint rng_state = pixel_idx + random_seed;

    vec3 origin = vec3(0.0);
    vec3 direction = normalize(vec3(
                coords.x * tan_fov_2,
                coords.y * tan_fov_2 / aspect_ratio,
                1.0));

    // Multisampling
    vec3 result = vec3(0.0);
    for (int sample_id = 0; sample_id < NUM_SAMPLES; sample_id++) {
        rng_state += sample_id;
        result += incident_light(origin, direction, rng_state);
    }
    result /= NUM_SAMPLES;

    return result;
}

void main() {
    tan_fov_2 = tan(fov / 2.0);
    FragColor = vec4(per_pixel(), 1.0);
}
