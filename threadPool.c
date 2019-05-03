// Aviad Gottesman 311250484
#include "threadPool.h"

void* managePool(void* pool) {

    struct thread_pool* threadPool = (struct thread_pool*) pool;

    while (!threadPool->isShuttingDown) {

        /* While queue is empty, wait for wakeup */
        while (osIsQueueEmpty(threadPool->taskQueue)) {

            /* Locking the mutex and waiting for tasks to enqueue */
            if (pthread_mutex_lock(threadPool->mutexEmptyQ) != 0) {
                fprintf(stderr, "Cannot lock mutex.\n");
            }

            if (pthread_cond_wait(threadPool->cv, threadPool->mutexEmptyQ) != 0) {
                fprintf(stderr, "Cannot wait for mutex.\n");
            }

            if (pthread_mutex_unlock(threadPool->mutexEmptyQ) != 0) {
                fprintf(stderr, "Cannot lock mutex.\n");
            }

        } /* osIsQueueEmpty */

    } /* isShuttingDown */

    return NULL;
}

/***
 * Create a new Thread Pool.
 * @param numOfThreads The number of threads in the pool.
 * @return A pointer to the new Thread Pool.
 */
ThreadPool* tpCreate(int numOfThreads) {

    // Allocate space in heap for struct.
    ThreadPool* threadPool;

    if ((threadPool = malloc(sizeof(ThreadPool))) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    }

    // Allocate space for thread-pool of struct.
    if ((threadPool->pool = malloc(sizeof(pthread_t) * numOfThreads)) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    }

    // Allocate space for thread-pool of struct.
    if ((threadPool->manager = malloc(sizeof(pthread_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    }

    if ((threadPool->mutexEmptyQ = malloc(sizeof(pthread_mutex_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    }

    if ((threadPool->cv = malloc(sizeof(pthread_cond_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    }

    // Save numOfThreads to struct.
    threadPool->numOfThreads = numOfThreads;

    // The tasks queue for the pool.
    threadPool->taskQueue = osCreateQueue();

    threadPool->isShuttingDown = false;

    /* Initialize the mutex. */
    pthread_mutex_init(threadPool->mutexEmptyQ, NULL);
    pthread_cond_init(threadPool->cv, NULL);

    /* Create and Start the pool manager. */
    pthread_create(threadPool->manager, NULL ,managePool, (void*)threadPool);

    return threadPool;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

    // Setting isShuttingDown to true, so we wont try to use it.
    threadPool->shouldWaitForTasks = shouldWaitForTasks;
    threadPool->isShuttingDown = true;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    return 1;
}

/***
 * Create a new TaskNode.
 * @param computeFunc The task.
 * @param param The parameters.
 * @return A pointer to the new TaskNode.
 */
task_node* tnCreate(void (*computeFunc) (void *), void* param) {
    task_node* taskNode = (task_node*) malloc(sizeof(task_node));
    if (taskNode == NULL) {
        return NULL;
    }

    taskNode->computeFunc = computeFunc;
    taskNode->parameters  = param;
    return taskNode;
}
