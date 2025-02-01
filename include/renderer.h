#pragma once

#include "scene.h"
#include <stdlib.h>

typedef struct {
    int screen_y;
    size_t framew, frameh;
    uint8_t *frame;
    scene *world;
} render_args;

void render_task(render_args *arg);
