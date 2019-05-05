// Aviad Gottesman 311250484
#include "threadPool.h"

// todo chage all errors to Error in system call

void tpFreeThreadPool(ThreadPool *threadPool);
void* tpRoutine(void *pool);

/***
 * Manage the Thread Pool.
 * @param pool The Thread Pool to manage.
 * @return Nothing.
 */
void* tpRoutine(void *pool) {

    // Try to get the ThreadPool.
    if (pool == NULL) {
        return NULL;
    }
    struct thread_pool* threadPool = (struct thread_pool*) pool;


    // Loop until we are requested to shutdown the ThreadPool.
    while (true) {

        /* Locking the mutex and waiting for tasks to enqueue */
        if (pthread_mutex_lock(threadPool->mutexEmptyQ) != 0) {
            fprintf(stderr, "Cannot lock mutex.\n");
        }

        /* While queue is empty, wait for wakeup */
        while (osIsQueueEmpty(threadPool->taskQueue) && !threadPool->isShuttingDown) {
            if (pthread_cond_wait(threadPool->cv, threadPool->mutexEmptyQ) != 0) {
                fprintf(stderr, "Cannot wait for mutex.\n");
            }
        }

        /*
         * If ThreadPool is shutting down and we need to wait
         * Check if we have tasks in queue:
         * if yes, run task
         * if no, close this thread.
         */
        if (threadPool->isShuttingDown && threadPool->shouldWaitForTasks) {

            if (osIsQueueEmpty(threadPool->taskQueue)) {
                /* Queue is empty, kill thread. */
                break;
            }

        } else if (threadPool->isShuttingDown && !threadPool->shouldWaitForTasks) {
            /* ThreadPool is shutting down and we do not need to wait, kill this thread. */
            break;
        }

        /*
         * Thread Pool is not shutting down OR shutting down and waiting,
         * Get task, un-lock mutex, run the task and free task allocated memory.
         */
        task_node* task = osDequeue(threadPool->taskQueue);
        if (pthread_mutex_unlock(threadPool->mutexEmptyQ) != 0) {
            fprintf(stderr, "Cannot un-lock mutex.\n");
        }
        (*(task->computeFunc))(task->parameters);
        free(task);

    }

    // Close this Thread.
    if (pthread_mutex_unlock(threadPool->mutexEmptyQ) != 0) {
        fprintf(stderr, "Cannot un-lock mutex.\n");
    }
    pthread_exit(NULL);
    return NULL;
}

/***
 * Create a new Thread Pool.
 * @param numOfThreads The number of threads in the pool.
 * @return A pointer to the new Thread Pool.
 */
ThreadPool* tpCreate(int numOfThreads) {

    ThreadPool* threadPool;

    // Allocate space in heap for struct.
    if ((threadPool = malloc(sizeof(ThreadPool))) == NULL) {
        fprintf(stderr, "Cannot allocate memory for ThreadPool.\n");
        return NULL;
    }

    // Allocate space for thread-threadArray of struct.
    if ((threadPool->threadArray = malloc(sizeof(pthread_t) * numOfThreads)) == NULL) {
        fprintf(stderr, "Cannot allocate memory for Thread array.\n");
        return NULL;
    } /* Clean array. */
    for (int i = 0; i < numOfThreads; ++i) {
        threadPool->threadArray[i] = NULL;
    }

    /*// Allocate space for thread-threadArray of struct.
    if ((threadPool->poolManager = malloc(sizeof(pthread_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory for manager.\n");
        return NULL;
    }*/

    if ((threadPool->mutexEmptyQ = malloc(sizeof(pthread_mutex_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory for mutex.\n");
        return NULL;
    }

    if ((threadPool->cv = malloc(sizeof(pthread_cond_t))) == NULL) {
        fprintf(stderr, "Cannot allocate memory for mutex cond.\n");
        return NULL;
    }

    // The tasks queue for the threadArray.
    threadPool->taskQueue = osCreateQueue();
    if ( threadPool->taskQueue == NULL) {
        fprintf(stderr, "Cannot allocate memory for queue.\n");
        return NULL;
    }

    /* Initialize the mutex. */
    pthread_mutex_init(threadPool->mutexEmptyQ, NULL);
    pthread_cond_init(threadPool->cv, NULL);

    // Save numOfThreads to struct.
    threadPool->numOfThreads = numOfThreads;


    threadPool->isShuttingDown = false;

    /* Create and Start the threadArray poolManager. */
    ///pthread_create(threadPool->poolManager, NULL, tpRoutine, (void *) threadPool);
    for (int i = 0; i < numOfThreads; ++i) {
        if ((threadPool->threadArray[i] = malloc(sizeof(pthread_t))) == NULL) {
            fprintf(stderr, "Cannot allocate memory for thread number %d in array.\n", i);
            return NULL;
        }
        pthread_create(threadPool->threadArray[i], NULL, tpRoutine, threadPool);
    }

    return threadPool;
}

/**
 * Notify pool manager to close the pool.
 * @param threadPool The Thread Pool to close.
 * @param shouldWaitForTasks The number of tasks to finish before closing.
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

    /* Do not allow two threads to close the same ThreadPool */
    if (threadPool->isShuttingDown) {
        return;
    }
    /* Locking the mutex. */
    if (pthread_mutex_lock(threadPool->mutexEmptyQ) != 0) {
        fprintf(stderr, "Cannot lock mutex in destroy.\n");
    }

    // Setting isShuttingDown to true, so we wont try to use it.
    threadPool->shouldWaitForTasks = shouldWaitForTasks == 0 ? false : true ;
    threadPool->isShuttingDown = true;

    /* Send broadcast to wake up all threads. */
    if ((pthread_cond_broadcast(threadPool->cv)) != 0) {
        fprintf(stderr, "Cannot send broadcast.\n");
    }
    /* Un-lock mutex. */
    if (pthread_mutex_unlock(threadPool->mutexEmptyQ) != 0) {
        fprintf(stderr, "Cannot unlock mutex in destroy.\n");
    }

    /* Join threads until all are done. */
    for (int i = 0; i < threadPool->numOfThreads; ++i) {

        if ((pthread_join(*threadPool->threadArray[i], NULL)) != 0) {
            fprintf(stderr, "Cannot join thread number %d.\n", i);
        }
    }

    tpFreeThreadPool(threadPool);
}

/***
 * Add a task to the queue:
 * Create Task.
 * Lock Mutex.
 * Add Task to queue.
 * Unlock Mutex.
 * @param threadPool The Thread Pool to do the task.
 * @param computeFunc The task.
 * @param param The parameters to the task.
 * @return -1 if failed, 0 if worked.
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

    /* If Thread Pool is closing down or NULL is passed, FAIL. */
    if (threadPool->isShuttingDown || threadPool == NULL || computeFunc == NULL) {
        fprintf(stderr, "Bad arguments for InsertTask or ThreadPool is shutting down.\n");
        return TASK_INSERT_FAILURE;
    }

    /* Create task_node struct. */
    task_node *taskNode = NULL;
    if ((taskNode = tnCreate(computeFunc, param)) == NULL) {
        fprintf(stderr, "Cannot create task to insert.\n");
        return TASK_INSERT_FAILURE;
    }

    /* Locking the mutex. */
    if (pthread_mutex_lock(threadPool->mutexEmptyQ) != 0) {
        fprintf(stderr, "Cannot lock mutex for task insert.\n");
        return TASK_INSERT_FAILURE;
    }

    /* Adding to queue. */
    osEnqueue(threadPool->taskQueue, taskNode);

    /* Notifying Threads that new task is available. */
    if (pthread_cond_signal(threadPool->cv) != 0) {
        fprintf(stderr, "Cannot signal task add.\n");
        return TASK_INSERT_FAILURE;
    }

    /* Un-locking the mutex. */
    if (pthread_mutex_unlock(threadPool->mutexEmptyQ) != 0) {
        fprintf(stderr, "Cannot un-lock mutex after task insert.\n");
        return TASK_INSERT_FAILURE;
    }

    return TASK_INSERT_SUCCESS;
}

/***
 * Create a new TaskNode.
 * @param computeFunc The task.
 * @param param The parameters.
 * @return A pointer to the new TaskNode.
 */
task_node* tnCreate(void (*computeFunc) (void *), void* param) {

    /* Allocate space. */
    task_node* taskNode = (task_node*) malloc(sizeof(task_node));
    if (taskNode == NULL) {
        return NULL;
    }

    /* Initiate struct. */
    taskNode->computeFunc = computeFunc;
    taskNode->parameters  = param;

    return taskNode;
}

/***
 * Free all allocated space for Thread Pool.
 * @param threadPool The Thread Pool to de-allocate space for.
 */
void tpFreeThreadPool(ThreadPool *threadPool) {

    if (threadPool == NULL) {
        return;
    }

    // Free all tasks and than the queue.
    while (!osIsQueueEmpty(threadPool->taskQueue)) {
        free(osDequeue(threadPool->taskQueue));
    }
    osDestroyQueue(threadPool->taskQueue);

    // Destroy and free pthread_cond_t
    pthread_cond_destroy(threadPool->cv);
    free(threadPool->cv);

    // Destroy and free mutex.
    pthread_mutex_destroy(threadPool->mutexEmptyQ);
    free(threadPool->mutexEmptyQ);

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