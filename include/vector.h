#pragma once

#include <stdlib.h>

typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} vector;

void vector_init(vector *v);
void vector_init_with_capacity(vector *v, size_t initial_capacity);
void vector_free(vector *v);

void vector_push(vector *v, void *element);
void *vector_pop(vector *v);
