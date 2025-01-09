#include "vector.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    vector threads;
    vector tasks;
    pthread_mutex_t tasks_mutex;
    bool threads_running;
} threadpool;

void threadpool_init(threadpool *pool, size_t num_threads);
void threadpool_free(threadpool *pool);

void threadpool_add_task(threadpool *pool, void (*f)(void *));
