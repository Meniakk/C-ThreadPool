#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define TASK_INSERT_FAILURE -1
#define TASK_INSERT_SUCCESS 0

/// Thread Pool struct.

typedef struct thread_pool
{
    pthread_t** threadArray;     /* An array of thread pointers. */
    pthread_mutex_t* mutexEmptyQ;/* The mutex to check for empty queue. */
    pthread_cond_t* cv;          /* The cond for the mutex. */
    struct os_queue* taskQueue;  /* The tasks queue. */
    bool isShuttingDown;         /* Is the thread threadArray being shutdown? */
    bool shouldWaitForTasks;      /* Should we wait for tasks in queue when shutting down? */
    int numOfThreads;            /* The number of threads in the threadArray. */

}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

/// Task Node struct.

typedef struct task_node {

    void (*computeFunc)(void *);
    void* parameters;

}task_node;

task_node* tnCreate(void (*computeFunc) (void *), void* param);

#endif
