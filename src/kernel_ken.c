#include "kernel_ken.h"
/*  ##############################################################################
     __  __      ___
    |__)|__)||\ | |
    |   | \ || \| |

    You can simply have your implementation of
    the functions wrapper the sys_call_monitor* functions that are provided
    as part of the initital kernel code.
*/

void print(char *string)
{
    monitor_write(string);
}

    // Output a null-terminated ASCII string to the monitor.

void print_hex(unsigned int n)
{
    monitor_write_hex(n);
}

    //Output an HEX integer to the monitor


void print_dec(unsigned int n)
{
    monitor_write_dec( n);
}

    // Output an integer to the monitor

/*  ##############################################################################
          __            __   __
    |\/| |_  |\/| |\/| /  \ |__) \_/
    |  | |__ |  | |  | \__/ | \   |

                         __   __       __      ___
    |\/|  /\  |\ |  /\  / _  |_  |\/| |_  |\ |  |
    |  | /--\ | \| /--\ \__) |__ |  | |__ | \|  |
*/
extern heap_t *kheap;
// extern heap_t *uheap;
void *alloc(unsigned int size, unsigned char page_align)
{
    return my_alloc(size, page_align, current_task->heap);
    //return my_alloc(size, page_align, uheap);
}

    // Allocates a contiguous region of memory 'size' in the user heap. If
    // page_align==1, it creates that block starting on a page boundary.


void free(void *p)
{
    my_free(p, current_task->heap);
    //my_free(p,uheap);
}

    // Releases a block from the user heap allocated with 'alloc'.
    // p is a reference to the memory to be deallocated.

/*  ##############################################################################
     __   __   __   __  __  __  __
    |__) |__) /  \ /   |_  (_  (_
    |    | \  \__/ \__ |__ __) __)

                         __   __       __      ___
    |\/|  /\  |\ |  /\  / _  |_  |\/| |_  |\ |  |
    |  | /--\ | \| /--\ \__) |__ |  | |__ | \|  |

    Processes will be scheduled according to priority. Priorities range from 1 to
    10. 1 is the highest. To prevent against high priority processes starving low
    priority processes, you are to implement dynamic priorities. At the expiration
    of each quantum, the scheduler can decrease the priority of the current
    running process (thereby penalizing it for taking that much CPU time).
    Eventually its priority will fall below that of the next highest process and
    that process will be allowed to run.

    Another approach is to have the scheduler keep track of low priority
    processes that do not get a chance to run and increase their priority so that
    eventually the priority will be high enough so that the processes will get
    scheduled to run. Once it runs for its quantum, the priority can be brought
    back to the previous low level.

    This periodic boosting of a process’ priority to ensure it gets a chance to
    run is called process aging. A simple way to implement aging is to simply
    increase every process’ priority and then have them get readjusted as they
    execute.
*/
// The next available process ID.
uint32_t next_pid = 1;
extern heap_t* uheap;
extern uint32_t last_heap_address;
extern uint32_t placement_address;
void initialise_tasking()
{
    // Rather important stuff happening, no interrupts please!
    monitor_write("INITIALISE TASKING\n");
    asm volatile("cli");

    // Relocate the stack so we know where it is.
    move_stack((void*)0xA0000000, 0x20000);

    // Initialise the first task (kernel task)
    current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
    current_task->id = next_pid++;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = current_directory;
    current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    current_task->next = 0;
    current_task->prev = 0;
    current_task->priority = 4;
    current_task->counter = 0;
    current_task->elapsed_tick = 0;
    current_task->state = TASK_READY;

    current_task->heap  = init_heap(HEAP_INDEX_SIZE);

    // Reenable interrupts.
    asm volatile("sti");
}

    // Initialises the tasking system. Any data structures or initialization of
    // the system that is necessary before your first call to fork() must be
    // done in this function. For example, establishing your ready queue.


int fork()
{
    // We are modifying kernel structures, and so cannot
    asm volatile("cli");
    print("IN FORK\n");
    print_dec(current_task->id);
    print("\n");
    // Take a pointer to this process' task struct for later reference.
    task_t *prev_task = (task_t*)current_task;
    // Clone the address space.
    page_directory_t *directory = clone_directory(current_directory);

    // Create a new process.
    task_t *new_task = (task_t*)kmalloc(sizeof(task_t));

    new_task->id = next_pid++;
    new_task->esp = new_task->ebp = 0;
    new_task->eip = 0;
    new_task->page_directory = directory;
    current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    new_task->next = 0;


    // Add it to the end of the ready queue.
    task_t *tmp_task = (task_t*)ready_queue;
    while (tmp_task->next)
        tmp_task = tmp_task->next;

    new_task->parent_id = tmp_task->id;
    new_task->state = TASK_READY;
    new_task->priority = (tmp_task->priority - 1 < 0) ? 1 :(tmp_task->priority - 1);
    new_task->rt_priority = new_task->priority;
    new_task->counter = 0;

    new_task->heap = init_heap(HEAP_INITIAL_SIZE);

    uint32_t eip = read_eip();
    // We could be the prev or the child here - check.
    if (current_task == prev_task)
    {
        // We are the prev, so set up the esp/ebp/eip for our child.
        uint32_t esp; asm volatile("mov %%esp, %0" : "=r"(esp));
        uint32_t ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
        new_task->esp = esp;
        new_task->ebp = ebp;
        new_task->eip = eip;
        tmp_task->next = new_task;
        new_task->prev = tmp_task;
        asm volatile("sti");
		    return new_task->id;
    }
    else
    {
        // We are the child.
        asm volatile("sti");
        return 0;
    }

}

    // Forks the current process, spawning a new one with a different
    // memory space. For the parent, the return value is the pid of
    // the new process. For the child, the return value is 0.


void exit(){
    asm volatile("cli");
    current_task->state = TASK_EXITED;
    asm volatile("sti");
    switch_task();
    return;
}

    // Terminates the current process, cleaning up the resources allocated
    // to the process.


void yield(){
    asm volatile("cli");
    task_t *tmp_task = (task_t *)ready_queue;
    task_t *next_task = (task_t *)current_task;
    int highest_pr = -1;
    // find task with highest pr
    while(tmp_task != 0){
        if (tmp_task->priority > highest_pr) {
            highest_pr = tmp_task->priority;
            next_task = tmp_task;
        }
        tmp_task = tmp_task->next;
    }
    switch_task();
    asm volatile("sti");
    //monitor_write("in yield, next task is ");
    //monitor_write_dec(next_task->id);
    //return next_task;
}


    // Causes the process to surrender the CPU. The result is that the process
    // with the highest priority is assigned the CPU. Note: This may be the
    // current process *if* it is the highest priority.

int sleep(unsigned int s)
{
    if(current_task->state == TASK_SLEEPING) {
      int diff =  current_task->sleep_end - get_timer_ticks();
      return (diff < 0)? 0 : diff/TICKPERSECOND;
    } else {
      task_t *current_task = (task_t *) get_current_task();

      current_task->state = TASK_SLEEPING;
      //current_task->sleep_start = timer_ticks;
      current_task->sleep_end = get_timer_ticks() + s*TICKPERSECOND;

      return s;
    }
}

    // Causes the process to surrender the CPU and go to sleep for n seconds.
    // The function returns 0 if the requested time has elapsed, or the
    // number of seconds left to sleep, if the call was interrupted by a
    // signal handler.


int getpid()
{
    return current_task->id;
}

    // Returns the pid of the current process.

int setpriority(int pid, int new_priority){
    task_t *tmp_task = (task_t*)ready_queue;
    while(tmp_task != 0){
        if (tmp_task->id == pid){
            int diff = new_priority - tmp_task->priority + tmp_task->rt_priority;
            tmp_task->priority = new_priority;
            tmp_task->rt_priority = (diff < 0 ) ? 0:diff;
            return new_priority;
        }
        tmp_task = tmp_task->next;
    }
    return 0;
}

    // Set the priority of the process. pid is the process id returned by
    // getpid(). newpriority is the new priority value between 1 and 10,
    // where 1 is highest priority. The return value is the resulting
    // priority of the pid. If the pid is invalid then the return value is 0.

/*  ##############################################################################
     __   __   __   __  __  __  __
    |__) |__) /  \ /   |_  (_  (_
    |    | \  \__/ \__ |__ __) __)

     __           __       __   __         ___      ___    __
    (_  \_/ |\ | /   |__| |__) /  \ |\ | |  _/  /\   |  | /  \ |\ |
    __)  |  | \| \__ |  | | \  \__/ | \| | /__ /--\  |  | \__/ | \|

    A semaphore must be initialized by open_sem() before it can be used.
    Processes waiting on a semaphore are resumed on a first-come first-served
    basis.
*/
int open_sem(int n)
{
    return sem_open(n);
}

    // n is the number of processes that can be granted access to the critical
    // region for this semaphore simultaneously. The return value is a semaphore
    // identifier to be used by signal and wait. 0 indicates open_sem failed.


int wait(int s)
{
    return sem_wait(s);
}

    // The invoking process is requesting to acquire the semaphore, s. If the
    // internal counter allows, the process will continue executing after acquiring
    // the semaphore. If not, the calling process will block and release the
    // processor to the scheduler. Returns semaphore id on success of acquiring
    // the semaphore, 0 on failure.


int signal(int s)
{
    return sem_signal(s);
}

    // The invoking process will release the semaphore, if and only if the process
    // is currently holding the semaphore. If a process is waiting on
    // the semaphore, the process will be granted the semaphore and if appropriate
    // the process will be given control of the processor, i.e. the waking process
    // has a higher scheduling precedence than the current process. The return value
    // is the seamphore id on success, 0 on failure.


int close_sem(int s)
{
    return sem_close(s);
}

    // Close the semaphore s and release any associated resources. If s is invalid then
    // return 0, otherwise the semaphore id.

/*  ##############################################################################
           ___  __  __   __   __   __   __  __  __  __
    | |\ |  |  |_  |__) |__) |__) /  \ /   |_  (_  (_
    | | \|  |  |__ | \  |    | \  \__/ \__ |__ __) __)

     __  __                         __      ___    __
    /   /  \ |\/| |\/| /  \ |\ | | /    /\   |  | /  \ |\ |
    \__ \__/ |  | |  | \__/ | \| | \__ /--\  |  | \__/ | \|

    pipes are first-in-first-out bounded buffers. Elements are read in the same
    order as they were written. When writes overtake reads, the first unread
    element will be dropped. Thus, ordering is always preserved.
    "read" and "write" on pipes are atomic, i.e., they are indivisible, and
    they are non-blocking. All pipes are of the same size.
*/
#define INVALID_PIPE -1

int open_pipe(){
    return pipe_new();
}

    // Initialize a new pipe and returns a descriptor. It returns INVALID_PIPE
    // when none is available.


unsigned int write(int fildes, const void *buf, unsigned int nbyte)
{
    return pipe_write(fildes,buf,nbyte);
}

    // Write the first nbyte of bytes from buf into the pipe fildes. The return value is the
    // number of bytes written to the pipe. If the pipe is full, no bytes are written.
    // Only write to the pipe if all nbytes can be written.


unsigned int read(int fildes, void *buf, unsigned int nbyte)
{
    return pipe_read(fildes,buf,nbyte);
}

    // Read the first nbyte of bytes from the pipe fildes and store them in buf. The
    // return value is the number of bytes successfully read. If the pipe is
    // invalid, it returns -1.


int close_pipe(int fildes)
{
   return pipe_free(fildes);
}

    // Close the pipe specified by fildes. It returns INVALID_PIPE if the fildes
    // is not valid.
