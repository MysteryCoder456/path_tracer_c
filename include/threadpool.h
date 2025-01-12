#include "vector.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    vector threads;
    vector tasks;
    pthread_mutex_t tasks_mutex;
    pthread_cond_t tasks_available_cond;
    pthread_cond_t tasks_exhausted_cond;
    bool threads_running;
} threadpool;

void threadpool_init(threadpool *pool, size_t num_threads);
void threadpool_destroy(threadpool *pool);

void threadpool_add_task(threadpool *pool, void (*f)(void *), void *arg);

void threadpool_wait_for_tasks(threadpool *pool);
