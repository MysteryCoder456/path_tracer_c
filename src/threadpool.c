#include "threadpool.h"
#include "vector.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>

// --- Private ---

typedef struct {
    void (*task)(void *);
    void *arg;
} threadpool_task;

void *threadpool_task_function(void *arg) {
    threadpool *pool = (threadpool *)arg;
    threadpool_task *task;

    while (pool->threads_running) {
        task = NULL;
        pthread_mutex_lock(&pool->tasks_lock);

        // Acquire a task, if any is available
        if (pool->tasks.size > 0)
            task = vector_pop(&pool->tasks);
        else
            pthread_mutex_unlock(&pool->tasks_exhausted_lock);

        pthread_mutex_unlock(&pool->tasks_lock);

        // Execute acquired task
        if (task != NULL) {
            task->task(task->arg);
            free(task);
        }
    }

    return NULL;
}

// --- Public ---

void threadpool_init(threadpool *pool, size_t num_threads) {
    // Initialize struct fields
    vector_init_with_capacity(&pool->threads, num_threads);
    vector_init(&pool->tasks);
    pthread_mutex_init(&pool->tasks_lock, NULL);
    pthread_mutex_init(&pool->tasks_exhausted_lock, NULL);
    pool->threads_running = true;

    // Create and initialize threads
    for (int i = 0; i < num_threads; i++) {
        pthread_t *new_thread = malloc(sizeof(pthread_t));
        vector_push(&pool->threads, new_thread);
        pthread_create(new_thread, NULL, threadpool_task_function, pool);
    }
}

void threadpool_destroy(threadpool *pool) {
    // Free threads
    pool->threads_running = false;
    for (int i = 0; i < pool->threads.size; i++) {
        pthread_t *thread = pool->threads.data[i];
        pthread_join(*thread, NULL);
        free(thread);
    }

    // Free unexecuted tasks
    for (int i = 0; i < pool->tasks.size; i++) {
        threadpool_task *task = pool->tasks.data[i];
        free(task);
    }

    // Free struct fields
    vector_free(&pool->threads);
    vector_free(&pool->tasks);
    pthread_mutex_destroy(&pool->tasks_lock);
    pthread_mutex_destroy(&pool->tasks_exhausted_lock);
}

void threadpool_add_task(threadpool *pool, void (*f)(void *), void *arg) {
    // Create a task
    threadpool_task *task = malloc(sizeof(threadpool_task));
    task->task = f;
    task->arg = arg;

    // Send created task to queue
    assert(pthread_mutex_trylock(&pool->tasks_exhausted_lock) != EINVAL);
    pthread_mutex_lock(&pool->tasks_lock);
    vector_push(&pool->tasks, task);
    pthread_mutex_unlock(&pool->tasks_lock);
}

void threadpool_wait_for_tasks(threadpool *pool) {
    pthread_mutex_lock(&pool->tasks_exhausted_lock);
    pthread_mutex_unlock(&pool->tasks_exhausted_lock);
}
