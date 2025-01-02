#pragma once

#include "cglm/types-struct.h"
#include "scene.h"
#include <stddef.h>

typedef struct {
    float distance;
    vec3s point;
    vec3s normal;
    size_t material;
} ray_hit;

ray_hit trace_ray(const vec3s origin, const vec3s direction,
                  const scene *world);
