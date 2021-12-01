#include "threadpool.h"

threadpool_t *create_threadpool(int max_threads_num)
{
    threadpool_t *threadpool = malloc(sizeof(threadpool_t));

    /* Initialize tasks queue */
    threadpool->tasks_front = NULL;
    threadpool->tasks_rear = NULL;

    /* Initialize tasks semaphore */
    pthread_mutex_init(&threadpool->task_access_lock, NULL);
    sem_init(&threadpool->tasks_sem, 0, 0);

    threadpool->worker_threads = malloc(sizeof(pthread_t) * max_threads_num);
    memset(threadpool->worker_threads, 0, sizeof(pthread_t) * max_threads_num);

    for (int i = 0; i < max_threads_num; i++) {
        pthread_create(&(threadpool->worker_threads[i]), NULL, threadpool_task_handler, threadpool);
        printf("thread %ld is ready to roll now.\n", threadpool->worker_threads[i]);
    }

    return threadpool;
}

void *threadpool_task_handler(void *pool)        // argument is pointer, so the value can be modified dynamically
{
    threadpool_t *threadpool = (threadpool_t *)pool;
    while (TRUE) {
        sem_wait(&threadpool->tasks_sem);   // sem_value--
        // break sem_wait 代表有task(資源)可以讓worker thread執行

        // 同時間只有一個thread可以去取得task、操作task queue
        pthread_mutex_lock(&threadpool->task_access_lock);
        threadpool_task_t *task = threadpool->tasks_front;
        threadpool->tasks_front = task->next;
        pthread_mutex_unlock(&threadpool->task_access_lock);
        
        task->function(task->args); // 執行任務function
        free(task);
    }
}

void threadpool_add_tasks(threadpool_t *threadpool, threadpool_task_t *task)
{
    task->next = threadpool->tasks_front;
    threadpool->tasks_front = task;
    sem_post(&threadpool->tasks_sem);
}