//
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "common.h"
#include "monitor.h"
#include <stddef.h>
#include <stdint.h>
#include "descriptor_tables.h"

extern void perform_task_switch(uint32_t, uint32_t, uint32_t, uint32_t);
heap_t *uheap=0;
extern uint32_t placement_address;


void move_stack(void *new_stack_start, uint32_t size)
{
  uint32_t i;
  // Allocate some space for the new stack.
  for( i = (uint32_t)new_stack_start;
       i >= ((uint32_t)new_stack_start-size);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
    alloc_frame( get_page(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
  }

  // Flush the TLB by reading and writing the page directory address again.
  uint32_t pd_addr;
  asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
  asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  uint32_t old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  uint32_t old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  uint32_t offset            = (uint32_t)new_stack_start - initial_esp;

  // New ESP and EBP.
  uint32_t new_stack_pointer = old_stack_pointer + offset;
  uint32_t new_base_pointer  = old_base_pointer  + offset;

  // Copy the stack.
  memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, initial_esp-old_stack_pointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.
  for(i = (uint32_t)new_stack_start; i > (uint32_t)new_stack_start-size; i -= 4)
  {
    uint32_t tmp = * (uint32_t*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      uint32_t *tmp2 = (uint32_t*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

void switch_task()
{
    asm volatile("cli");
    //monitor_write_dec(current_task->id);
    //monitor_write("\n");
    // If we haven't initialised tasking yet, just return.
    if (!current_task)
        return;

    // Read esp, ebp now for saving later on.
    uint32_t esp, ebp, eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));

    // Read the instruction pointer. We do some cunning logic here:
    // One of two things could have happened when this function exits -
    //   (a) We called the function and it returned the EIP as requested.
    //   (b) We have just switched tasks, and because the saved EIP is essentially
    //       the instruction after read_eip(), it will seem as if read_eip has just
    //       returned.
    // In the second case we need to return immediately. To detect it we put a dummy
    // value in EAX further down at the end of this function. As C returns values in EAX,
    // it will look like the return value is this dummy value! (0x12345).
    eip = read_eip();

    // Have we just switched tasks?
    if (eip == 0x12345){
      asm volatile("sti");
        return;
    }

    // No, we didn't switch tasks. Let's save some register values and switch.
    current_task->eip = eip;
    current_task->esp = esp;
    current_task->ebp = ebp;

    current_task = pick_next_task();
    current_task->counter ++;
    current_task->rt_priority --;

    eip = current_task->eip;
    esp = current_task->esp;
    ebp = current_task->ebp;

    // Make sure the memory manager knows we've changed page directory.
    current_directory = current_task->page_directory;
    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);
    // Here we:
    // * Stop interrupts so we don't get interrupted.
    // * Temporarily puts the new EIP location in ECX.
    // * Loads the stack and base pointers from the new task struct.
    // * Changes page directory to the physical address (physicalAddr) of the new directory.
    // * Puts a dummy value (0x12345) in EAX so that above we can recognise that we've just
    //   switched task.
    // * Restarts interrupts. The STI instruction has a delay - it doesn't take effect until after
    //   the next instruction.
    // * Jumps to the location in ECX (remember we put the new EIP in there).

    perform_task_switch(eip, current_directory->physicalAddr, ebp, esp);
    asm volatile("sti");
}


void show_tasks(){
    asm volatile("cli");
    monitor_write("SHOW TASKS\n");
    task_t *tmp_task = (task_t*)ready_queue;
    while (tmp_task){
        monitor_write("id / parent id ");
        monitor_write_dec(tmp_task->id);
        monitor_write("/");
        monitor_write_dec(tmp_task->parent_id);
        monitor_write("\n");
        monitor_write("task state : ");
        monitor_write_dec(tmp_task->state);
        monitor_write("\n");
        print("task priority: ");
        print_dec(tmp_task->priority);
        print("\n");
        tmp_task = tmp_task->next;

    }
    asm volatile("sti");
    return;
}

task_t *get_current_task(){
    return (task_t*)current_task;
}

void reset_rt_priority(){
    asm volatile("cli");
    task_t *tmp_task = (task_t*)ready_queue;
    while(tmp_task != 0){
        tmp_task->rt_priority = tmp_task->priority;
        tmp_task = tmp_task->next;
    }
    asm volatile("sti");
}

task_t *pick_next_task(){
  asm volatile("cli");
    task_t *tmp_task = (task_t*)ready_queue;
    task_t *next_task = 0;
    int highest_priority = 0;
    // get next ready task.
    while(tmp_task != 0){
        if (tmp_task->state == TASK_EXITED){
            task_t *exit_task = tmp_task;
            tmp_task = tmp_task->next;
            exit_task->prev->next = exit_task->next;
            exit_task->next->prev = exit_task->prev;
            exit_task->next->parent_id = ready_queue->id;
            kfree(exit_task->page_directory);
            kfree(exit_task);
            if(tmp_task == 0) break;
        }
        if (tmp_task->state == TASK_SLEEPING){
            if(tmp_task->sleep_end < get_timer_ticks()){
                tmp_task->state = TASK_READY;
            }
        }
        if (tmp_task->state == TASK_READY && tmp_task->rt_priority != 0 && tmp_task->priority > highest_priority) {
            highest_priority = tmp_task->priority;
            next_task = tmp_task;
        }
        tmp_task = tmp_task->next;
    }
    // all rt_pr s are zero. reset rt_pr
    if (next_task == 0){
        reset_rt_priority();
        asm volatile("sti");
        return pick_next_task();
    }
    asm volatile("sti");
    return next_task;
}
