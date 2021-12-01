#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0

/**
 *  The threadpool task data structure
 */
struct threadpool_task {
    struct threadpool_task *next;
    void (*function)(void *);
    void *args; // the arguments passed to thread
} ;
typedef struct threadpool_task threadpool_task_t;

/**
 *  The threadpool data structure
 */
typedef struct {
    /* pthreads */
    pthread_t *worker_threads;

    /* tasks */
    threadpool_task_t *tasks_front;   // the front of waiting queue of threadpool tasks
    threadpool_task_t *tasks_rear;   // the rear of waiting queue of threadpool tasks

    /* The synchronous locks */
    sem_t tasks_sem;    // the available tasks for threads
    pthread_mutex_t task_access_lock;  // mutex for reading/writing value, task access lock
} threadpool_t;

/* Create the threadpool and init the threads */
threadpool_t *create_threadpool(int); 

/* Execute the task until semaphore greater than 0 */
void *threadpool_task_handler(void *);

/* Add the tasks(from the client) to the task waiting queue */
void threadpool_add_tasks(threadpool_t *, threadpool_task_t *);

#endif  // THREADPOOL_H_