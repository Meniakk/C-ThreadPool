// Aviad Gottesman 311250484
#include "threadPool.h"

void freeThreadPool(ThreadPool* threadPool);
void* managePool(void* pool);

/***
 * Manage the Thread Pool.
 * @param pool The Thread Pool to manage.
 * @return Nothing.
 */
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

            // If we got the signal in order to close the threadArray, break.
            if (threadPool->isShuttingDown) {
                break;
            }

        } /* osIsQueueEmpty */

        // If we got the signal in order to close the threadArray, break.
        if (threadPool->isShuttingDown) {
            break;
        }

        //todo Do all the jobs you can threadArray master. (Queue is not empty for-sure)


    } /* isShuttingDown */

    //todo Close the threadArray, do shouldWaitForTasks jobs and kill yourself.
    freeThreadPool(threadPool);

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

    // Allocate space for thread-threadArray of struct.
    if ((threadPool->threadArray = malloc(sizeof(pthread_t) * numOfThreads)) == NULL) {
        fprintf(stderr, "Cannot allocate memory.\n");
        return NULL;
    } /* Clean array. */
    for (int i = 0; i < numOfThreads; ++i) {
        threadPool->threadArray[i] = NULL;
    }

    // Allocate space for thread-threadArray of struct.
    if ((threadPool->poolManager = malloc(sizeof(pthread_t))) == NULL) {
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

    // The tasks queue for the threadArray.
    threadPool->taskQueue = osCreateQueue();

    threadPool->isShuttingDown = false;

    /* Initialize the mutex. */
    pthread_mutex_init(threadPool->mutexEmptyQ, NULL);
    pthread_cond_init(threadPool->cv, NULL);

    /* Create and Start the threadArray poolManager. */
    pthread_create(threadPool->poolManager, NULL ,managePool, (void*)threadPool);

    return threadPool;
}

/**
 * Notify pool manager to close the pool.
 * @param threadPool The Thread Pool to close.
 * @param shouldWaitForTasks The number of tasks to finish before closing.
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

    // Setting isShuttingDown to true, so we wont try to use it.
    threadPool->shouldWaitForTasks = shouldWaitForTasks;
    threadPool->isShuttingDown = true;

    // Send signal in case thread is waiting.
    pthread_cond_signal(threadPool->cv);
}

/***
 * Add a task to the queue.
 * @param threadPool The Thread Pool to do the task.
 * @param computeFunc The task.
 * @param param The parameters to the task.
 * @return //todo Write
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

    /* If Thread Pool is closing down, do not add tasks. */
    if (threadPool->isShuttingDown) {
        return -1;
    }
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

/***
 * Free all allocated space for Thread Pool.
 * @param threadPool The Thread Pool to de-allocate space for.
 */
void freeThreadPool(ThreadPool* threadPool) {

    //todo Free the task queue.
    while (osIsQueueEmpty(threadPool->taskQueue)) {
        if (threadPool->taskQueue->head != NULL) {
            free(threadPool->taskQueue->head);
        }
    }
    osDestroyQueue(threadPool->taskQueue);

    // Destroy and free pthread_cond_t
    pthread_cond_destroy(threadPool->cv);
    free(threadPool->cv);

    // Destroy and free mutex.
    pthread_mutex_destroy(threadPool->mutexEmptyQ);

    // Free poolManager.
    free(threadPool->poolManager);

    // Free Thread in array and than free the Array.
    for (int i = 0; i < threadPool->numOfThreads; ++i) {
        if (threadPool->threadArray[i] != NULL) {
            free(threadPool->threadArray[i]);
        }
    }
    free(threadPool->threadArray);

    // Free struct.
    free(threadPool);
}