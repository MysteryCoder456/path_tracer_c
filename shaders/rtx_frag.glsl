#version 410

#define M_PI 3.1415926535897932384626433832795
#define MAX_BOUNCES 4

in vec2 coords;
out vec4 FragColor;

const float aspect_ratio = 1280.0 / 800.0; // HACK: turn into uniform
const float fov = 90 * M_PI / 180.0; // HACK: turn into uniform
const float tan_fov_2 = tan(fov / 2.0);

vec3 incident_light(vec3 origin, vec3 direction, int bounces) {
    return vec3(1.0);
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
    FragColor = vec4(per_pixel(), 1.0);
}
