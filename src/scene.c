#include "scene.h"
#include <stdlib.h>

// --- Private ---

void scene_extend_materials(scene *s) {
    s->max_materials *= 2;
    s->materials =
        realloc(s->materials, s->max_materials * sizeof(shape_material));
}

void scene_extend_objects(scene *s) {
    s->max_objects *= 2;
    s->objects = realloc(s->objects, s->max_objects * sizeof(shape));
}

// --- Public ---

void scene_init(scene *s) {
    s->max_objects = 64;
    s->objects = calloc(s->max_objects, sizeof(shape));
    s->num_objects = 0;

    s->max_materials = 16;
    s->materials = calloc(s->max_materials, sizeof(shape_material));
    s->num_materials = 0;
}

size_t scene_add_material(scene *s, const vec3s albedo, const float roughness,
                          const float metallicity, const vec3s emission_color,
                          const float emission_strength) {
    if (s->num_materials == s->max_materials)
        scene_extend_materials(s);

    s->materials[s->num_materials++] = (shape_material){
        .albedo = albedo,
        .roughness = roughness,
        .metallicity = metallicity,
        .emission_color = emission_color,
        .emission_strength = emission_strength,
    };
    return s->num_materials - 1;
}

void scene_add_sphere(scene *s, const vec3s center, const float radius,
                      const size_t material) {
    if (s->num_objects == s->max_objects)
        scene_extend_objects(s);

    s->objects[s->num_objects++] = (shape){
        .tag = SPHERE,
        .material = material,
        .sphere = {.center = center, .radius = radius},
    };
}

void scene_add_triangle(scene *s, const vec3s v0, const vec3s v1,
                        const vec3s v2, const size_t material) {
    if (s->num_objects == s->max_objects)
        scene_extend_objects(s);

    s->objects[s->num_objects++] = (shape){
        .tag = TRIANGLE,
        .material = material,
        .triangle = {.v0 = v0, .v1 = v1, .v2 = v2},
    };
}

void scene_free(scene *s) {
    free(s->objects);
    free(s->materials);
}
