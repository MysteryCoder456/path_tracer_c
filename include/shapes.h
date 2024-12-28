#include "cglm/types-struct.h"

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
    vec3s ambient;
    vec3s diffuse;
    vec3s specular;
} shape_material;

typedef struct {
    enum { SPHERE, TRIANGLE } tag;
    shape_material material;
    union {
        sphere sphere;
        triangle triangle;
    };
} shape;
