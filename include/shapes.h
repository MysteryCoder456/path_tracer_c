#pragma once

#include "cglm/types-struct.h"
#include <stddef.h>

typedef struct {
    vec3s center;
    float radius;
} sphere;

typedef struct {
    vec3s v0;
    vec3s v1;
    vec3s v2;
} triangle;

typedef struct {
    vec3s albedo;
    float roughness;
    float metallicity;
} shape_material;

typedef struct {
    enum { SPHERE, TRIANGLE } tag;
    size_t material;
    union {
        sphere sphere;
        triangle triangle;
    };
} shape;
