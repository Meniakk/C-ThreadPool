#include "threadPool.h"
#include "osqueue.h"
#include <stdio.h>

void PrintHello (void* a)
{
    printf("hello\n");
}

int main() {

    struct thread_pool* pool = tpCreate(2);

    int i = 0;
    while (i == 0) {
        i = 0;
    }
    pthread_cond_signal(pool->cv);

    while (true) {}
}