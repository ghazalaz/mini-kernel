//
// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "common.h"

#include <stddef.h>
#include <stdint.h>
#include "paging.h"
#include "timer.h"
#include "ordered_array.h"
#include "kheap.h"

#define KERNEL_STACK_SIZE 2024
#define TASK_HEAP_SIZE 0x1000


enum task_state {
  TASK_RUNNING = 1,
  TASK_READY = 2,
  TASK_SLEEPING = 3,
  TASK_INTERRUPTABLE = 4,
  TASK_UNINTERRUPTABLE = 5,
  TASK_EXITED = 6,
  TASK_ZOMBOE = 7,
};



// This structure defines a 'task' - a process.
typedef struct task
{
    int id;                // Process ID.
    uint32_t esp, ebp;       // Stack and base pointers.
    uint32_t eip;            // Instruction pointer.
    page_directory_t *page_directory; // Page directory.
    struct task *next;     // The next task in a linked list.
    struct task *prev;          // The previous task in linked list
    uint32_t kernel_stack;   // Kernel stack location
    uint8_t state;      // state of the task
    int parent_id;      // parent id
    //int sleep_start;    // sleep start tick;
    int sleep_end;     // time to wakeup;
    int priority;
    int rt_priority;
    int counter;
    int elapsed_tick;
    heap_t *heap;

} task_t;

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;
// The start of the heap linked list.

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern uint32_t initial_esp;
extern uint32_t read_eip();
extern heap_t *kheap;

uint32_t last_heap_address;

// Called by the timer hook, this changes the running process.
void switch_task();

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(void *new_stack_start, uint32_t size);

void switch_to_user_mode();

void show_tasks();

task_t * get_current_task();

task_t *pick_next_task();

void reset_rt_pr();

#endif
