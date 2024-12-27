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

typedef enum { SPHERE, TRIANGLE } shape_tag;

typedef struct {
    shape_tag tag;
    vec3s color; // TODO: change to something like a material
    union {
        sphere sphere;
        triangle triangle;
    };
} shape;
