#ifndef WORK_QUEUE_LINUX_H
#define WORK_QUEUE_LINUX_H

#include <pthread.h>
#include <semaphore.h>

struct WQ_Queue {
    WQ_Work buffer[256];
    volatile u32 completion_count;
    volatile u32 target_completion_count;

    volatile u32 next_idx_to_write;
    volatile u32 next_idx_to_read;

    sem_t semaphore;
};

#endif // WORK_QUEUE_LINUX_H
