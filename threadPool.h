#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct thread_pool
{
    pthread_t** threadArray;     /* An array of thread pointers. */
    pthread_t* poolManager;      /* The main thread of the threadArray. */
    pthread_mutex_t* mutexEmptyQ;/* The mutex to check for empty queue. */
    pthread_cond_t* cv;          /* The cond for the mutex. */
    struct os_queue* taskQueue;  /* The tasks queue. */
    bool isShuttingDown;         /* Is the thread threadArray being shutdown? */
    int numOfThreads;            /* The number of threads in the threadArray. */
    int shouldWaitForTasks;      /* The number of threads to wait for. */

}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

typedef struct task_node {

    void (*computeFunc)(void *);
    void* parameters;

}task_node;

task_node* tnCreate(void (*computeFunc) (void *), void* param);

#endif
