#ifndef SEMAPHOR_H
#define SEMAPHOR_H
#include "task.h"

#define TOTAL_SEMAPHORS 10

typedef struct waiting_tasks {
    int id;
    task_t* task;
    struct waiting_tasks *next;
} waiting_tasks_t;

typedef struct semaphore {
    int8_t opened;
    int counter;
    waiting_tasks_t* waiting_list;
} semaphore_t;



semaphore_t semaphore_array[TOTAL_SEMAPHORS];

int sem_open(int n);

int sem_wait(int s);

int sem_signal(int s);

int sem_close(int s);

#endif