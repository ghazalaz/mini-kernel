#include "semaphore.h"
#include <stdbool.h>
int sem_open(int n)
{
  int i;
    for(i=1;i<TOTAL_SEMAPHORS+1;i++) {
        if(semaphore_array[i].opened!=1) {
            semaphore_array[i].opened = 1;
            semaphore_array[i].counter = n;
            semaphore_array[i].waiting_list = 0;
            return i;
        }
    }
    return 0;
}

int sem_wait(int s) {
    if(semaphore_array[s].opened == 1) {
        // Rather important stuff happening, no interrupts please!
        asm volatile("cli");
        if(semaphore_array[s].counter > 0) {
            semaphore_array[s].counter--;
        } else {
            task_t * current_task = get_current_task();
            current_task->state = TASK_INTERRUPTABLE;
            waiting_tasks_t* waiting_tasks = semaphore_array[s].waiting_list;
            while(waiting_tasks) {
                waiting_tasks = waiting_tasks->next;
            }
            waiting_tasks = (waiting_tasks_t*)kmalloc(sizeof(waiting_tasks_t));
            waiting_tasks->id = current_task->id;
            waiting_tasks->task = current_task;
            waiting_tasks->next = 0;
            semaphore_array[s].waiting_list = waiting_tasks;
        }
        // Reenable interrupts.
        asm volatile("sti");
        switch_task();
        return s;
    }
    return 0;
}

int sem_signal(int s) {
    if(semaphore_array[s].opened == 1) {
        if(semaphore_array[s].counter == 0) {
            // Rather important stuff happening, no interrupts please!
            asm volatile("cli");
            waiting_tasks_t* found = semaphore_array[s].waiting_list;
            if(found!=0) {
                semaphore_array[s].waiting_list  = found->next;
                found->task->state = TASK_READY;
                kfree(found);
            } else {
                semaphore_array[s].counter++;
            }
            // Reenable interrupts.
            asm volatile("sti");
        } else {
            semaphore_array[s].counter++;
        }
    }
    return 0;
}

int sem_close(int s) {
    if(semaphore_array[s].opened == 1) {
        waiting_tasks_t* waiting_tasks = semaphore_array[s].waiting_list;
        while(waiting_tasks) {
            waiting_tasks->task->state = TASK_READY;
            waiting_tasks = waiting_tasks->next;
        }
        switch_task();
        semaphore_array[s].opened = false;
        semaphore_array[s].counter = 0;
        semaphore_array[s].waiting_list = 0;
        return s;
    }
    return 0;
}
