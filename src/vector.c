#include "vector.h"

void vector_init(vector *v) { vector_init_with_capacity(v, 8); }

void vector_init_with_capacity(vector *v, size_t initial_capacity) {
    v->capacity = initial_capacity;
    v->size = 0;
    v->data = malloc(v->capacity * sizeof(void *));
}

void vector_free(vector *v) { free(v->data); }

void vector_push(vector *v, void *element) {
    // If vector is full, resize it to add new element
    if (v->size == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, v->capacity * sizeof(void *));
    }

    // Push new element to the back
    v->data[v->size++] = element;
}

void *vector_pop(vector *v) {
    // Check if there are any elements to pop
    if (v->size <= 0)
        return NULL;

    // Pop an element from the back
    return v->data[--v->size];
}
