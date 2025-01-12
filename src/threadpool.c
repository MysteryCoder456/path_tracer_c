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
        pthread_mutex_lock(&pool->tasks_mutex);

        // Wait for and acquire a task
        if (pool->tasks.size == 0) {
            pthread_cond_signal(&pool->tasks_exhausted_cond);
            pthread_cond_wait(&pool->tasks_available_cond, &pool->tasks_mutex);
        }
        task = vector_pop(&pool->tasks);

        pthread_mutex_unlock(&pool->tasks_mutex);

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
    pthread_mutex_init(&pool->tasks_mutex, NULL);
    pthread_cond_init(&pool->tasks_available_cond, NULL);
    pthread_cond_init(&pool->tasks_exhausted_cond, NULL);
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
    pthread_cond_signal(&pool->tasks_available_cond); // LIE to the threads!
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
    pthread_mutex_destroy(&pool->tasks_mutex);
    pthread_cond_destroy(&pool->tasks_available_cond);
    pthread_cond_destroy(&pool->tasks_exhausted_cond);
}

void threadpool_add_task(threadpool *pool, void (*f)(void *), void *arg) {
    // Create a task
    threadpool_task *task = malloc(sizeof(threadpool_task));
    task->task = f;
    task->arg = arg;

    // Send created task to queue
    pthread_mutex_lock(&pool->tasks_mutex);
    vector_push(&pool->tasks, task);
    pthread_cond_signal(&pool->tasks_available_cond);
    pthread_mutex_unlock(&pool->tasks_mutex);
}

void threadpool_wait_for_tasks(threadpool *pool) {
    pthread_mutex_lock(&pool->tasks_mutex);
    pthread_cond_wait(&pool->tasks_exhausted_cond, &pool->tasks_mutex);
    pthread_mutex_unlock(&pool->tasks_mutex);
}
