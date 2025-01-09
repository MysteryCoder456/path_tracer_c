#include "threadpool.h"
#include "vector.h"
#include <pthread.h>

// --- Private ---

void *threadpool_task(void *arg) {
    threadpool *pool = (threadpool *)arg;
    void (*task)(void *);

    while (pool->threads_running) {
        task = NULL;
        pthread_mutex_lock(&pool->tasks_mutex);

        // Acquire a task, if any is available
        if (pool->tasks.size > 0) {
            task = vector_pop(&pool->tasks);
        }

        pthread_mutex_unlock(&pool->tasks_mutex);

        // Execute acquired task
        if (task != NULL)
            // TODO: replace with parameters
            task(NULL);
    }

    return NULL;
}

// --- Public ---

void threadpool_init(threadpool *pool, size_t num_threads) {
    // Initialize struct fields
    vector_init_with_capacity(&pool->threads, num_threads);
    vector_init(&pool->tasks);
    pthread_mutex_init(&pool->tasks_mutex, NULL);
    pool->threads_running = true;

    // Create and initialize threads
    for (int i = 0; i < num_threads; i++) {
        pthread_t *new_thread = malloc(sizeof(pthread_t));
        vector_push(&pool->threads, new_thread);
        pthread_create(new_thread, NULL, threadpool_task, pool);
    }
}

void threadpool_free(threadpool *pool) {
    // Free elements in threads vector
    pool->threads_running = false;
    for (int i = 0; i < pool->threads.size; i++) {
        pthread_t *thread = pool->threads.data[i];
        pthread_join(*thread, NULL);
        free(thread);
    }

    // Free struct fields
    vector_free(&pool->threads);
    vector_free(&pool->tasks);
    pthread_mutex_destroy(&pool->tasks_mutex);
}

void threadpool_add_task(threadpool *pool, void (*f)(void *)) {
    pthread_mutex_lock(&pool->tasks_mutex);
    vector_push(&pool->tasks, f);
    pthread_mutex_unlock(&pool->tasks_mutex);
}
