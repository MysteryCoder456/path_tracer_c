#pragma once

#include "cglm/types-struct.h"
#include "shapes.h"

typedef struct {
    shape *objects;
    size_t num_objects;
    size_t max_objects;
    shape_material *materials;
    size_t num_materials;
    size_t max_materials;
    vec3s light_direction;
    vec3s light_color;
    vec3s sky_color;
} scene;

void scene_init(scene *s);

size_t scene_add_material(scene *s, const vec3s albedo, const float roughness,
                          const float metallicity, const vec3s emission_color,
                          const float emission_strength);

void scene_add_sphere(scene *s, const vec3s center, const float radius,
                      const size_t material);
void scene_add_triangle(scene *s, const vec3s v0, const vec3s v1,
                        const vec3s v2, const size_t material);

void scene_free(scene *s);
