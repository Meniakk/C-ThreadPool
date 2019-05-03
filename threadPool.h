#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct thread_pool
{
    pthread_t* pool;            /* The array of threads. */
    pthread_t* manager;         /* The main thread of the pool. */
    int numOfThreads;           /* The number of threads in the pool. */
    int shouldWaitForTasks;     /* The number of threads to wait for. */
    bool isShuttingDown;        /* Is the thread pool being shutdown? */
    struct os_queue* taskQueue; /* The tasks queue. */
    pthread_mutex_t* mutexEmptyQ;/* The mutex to check for empty queue. */
    pthread_cond_t* cv;

}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

void* managePool(void* pool);

typedef struct task_node {

    void (*computeFunc)(void *);
    void* parameters;

}task_node;

task_node* tnCreate(void (*computeFunc) (void *), void* param);

#endif
