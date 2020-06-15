#include "kernel_ken.h"
#include "user_app.h"
#include "task.h"
#include "kheap.h"

extern volatile task_t *current_task;
extern uint32_t current_number_of_tasks;
#define BOUNDED_BUFFER_SIZE (PAGE_SZ - 2) // Nearly the size of a page

void print_fucntion_details(char* func_name, int line){
    print("\n");
    print(__FILE__);
    print("[");
    print_dec(line);
    print("]: ");
    print(func_name);
}
void test_fork(){
    fork();
    int pid = getpid();
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Fork pid: ");
    print_dec(pid);
    print("\n");
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task Prio is: ");
    print_dec(current_task->priority);
    print("\n");

    fork();

    pid = getpid();
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Fork Child pid: ");
    print_dec(pid);
    print("\n");

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task Prio is:  ");
    print_dec(current_task->priority);
    print("\n");

    yield();

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task Prio After yield() is:  ");
    print_dec(current_task->priority);
    print("\n");

    int prio = setpriority(getpid(),1);

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Priority of Task ");
    print_dec(pid);
    print(" After setpriority is : ");
    print_dec(prio);
    print("\n");

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task Prio is: ");
    print_dec(current_task->priority);
    print("\n");


    print_fucntion_details((char *)__FUNCTION__,__LINE__);;
    print(": Calling yield():\n ");

    yield();

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task Prio After yield() is:  ");
    print_dec(current_task->priority);
    print("\n");
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Current Task pid: ");
    print_dec(getpid());
    print("\n");
    // unsigned int time_to_sleep = 5;
    // int remaining_sleep_time = -1;
    //
    // print_fucntion_details((char *)__FUNCTION__,__LINE__);
    // print("): Call sleep(");
    // print_dec(time_to_sleep);
    // print(").\n");
    // remaining_sleep_time = sleep(time_to_sleep);
    //
    // print_fucntion_details((char *)__FUNCTION__,__LINE__);
    // print(": Back from Call sleep(");
    // print_dec(time_to_sleep);
    // print("): remaining_sleep_time: ");
    // print_dec(remaining_sleep_time);
    // print(".\n");

    if (getpid() > 1){
      print_fucntion_details((char *)__FUNCTION__,__LINE__);
      print(": Exiting Task ");
      print_dec(getpid());
      print("\n");
      exit();
    }
    return;
}

void test_sem(){


    int sem = open_sem(2);
    int wsem;
    int csem;

    fork();

    wsem = wait(sem);
    csem = close_sem(sem);

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Closing sem : ");
    print_dec(csem);
    print("\n");

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Task ");
    print_dec(getpid());
    print(" is Waiting on Closed sem: ");
    print_dec(csem);
    print("\n");
    wsem = wait(csem);

    if(wsem == 0){
        print("\nTEST SUCCEED : close_sem closes all semaphores");
    }else{
        print("TEST FAILS : close_sem should close all the semaphores. wait() shouldn't work on closed sem.\n");
    }


}

void test_pipe(){

    int pid = fork();
    int pipe_id = open_pipe();
    int nbytes = 15;
    char *buffer = "BUFFER CONTENT";
    char* read_buf = "12345678912345";
    uint32_t val;

    /*
        OPEN A PIPE

    */

    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    int nwbytes = write(pipe_id,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id);
    print(" buffer: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nwbytes);
    print("\n");


    unsigned int bytes_read_from_pipe = read(pipe_id, read_buf, nbytes);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Call Read(): pipe_id: ");
    print_dec(pipe_id);
    print(" read_buf: '");
    print(read_buf);
    print("' nbyte: ");
    print_dec(nbytes);
    print(" bytes_read_from_pipe: ");
    print_dec(bytes_read_from_pipe);
    print("\n");

    /*
        CLOSE THE PIPE

    */

    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Closing pipe: pipe_id: ");
    print_dec(pipe_id);
    print("\n");
    close_pipe(pipe_id);

    buffer = "SOMETHINGELSE";
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    nwbytes = write(pipe_id,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id);
    print(" buffer: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nwbytes);
    print("\n");

    bytes_read_from_pipe = read(pipe_id, read_buf, nbytes);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Call Read(): pipe_id: ");
    print_dec(pipe_id);
    print(" read_buf: '");
    print(read_buf);
    print("' nbyte: ");
    print_dec(nbytes);
    print(" bytes_read_from_pipe: ");
    print_dec(bytes_read_from_pipe);
    print("\n");

    /*
        OPEN A SECOND PIPE

        ClOSE THE SECOND PIPE

    */

    int pipe_id2 = open_pipe();

    buffer = "SOMETHINGELSE";
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    nwbytes = write(pipe_id2,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id2);
    print(" buffer: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nwbytes);
    print("\n");

    bytes_read_from_pipe = read(pipe_id2, read_buf, nbytes);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Call Read(): pipe_id: ");
    print_dec(pipe_id2);
    print(" read_buf: '");
    print(read_buf);
    print("' nbyte: ");
    print_dec(nbytes);
    print(" bytes_read_from_pipe: ");
    print_dec(bytes_read_from_pipe);
    print("\n");

    val = close_pipe(pipe_id2);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Closing pipe: pipe_id: ");
    print_dec(pipe_id2);
    print(": Return  ");
    print_dec(val);
    print("\n");

    buffer = "SOMETHINGELSE";
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    nwbytes = write(pipe_id2,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id2);
    print(" buffer: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nwbytes);
    print("\n");

    bytes_read_from_pipe = read(pipe_id2, read_buf, nbytes);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Call Read(): pipe_id: ");
    print_dec(pipe_id2);
    print(" read_buf: '");
    print(read_buf);
    print("' nbyte: ");
    print_dec(nbytes);
    print(" bytes_read_from_pipe: ");
    print_dec(bytes_read_from_pipe);
    print("\n");


    /*
        OPEN PIPE
        FILL THE PIPE'S BUFFER
        THEN READ

    */

    int pipe_id3 = open_pipe();
    buffer = "FIRST BUFFER CONTENT";
    nbytes = 20;
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    int i = 0;
    for (i = 0; i< BOUNDED_BUFFER_SIZE; i+=20){
        nwbytes = write(pipe_id3,buffer,nbytes);

    }
    buffer = "LASTT BUFFER CONTENT";
    nwbytes = write(pipe_id3,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id3);
    print(" buffer: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nwbytes);
    print("\n");

    /*
        READ FROM PIPE MORE THAN WRITTEN

    */
    int pipe_id4 = open_pipe();
    buffer = "12345";
    nwbytes = write(pipe_id4,buffer,nbytes);
    print(": Call Write(): pipe_id: ");
    print_dec(pipe_id4);
    print(" buffer: '");
    print(buffer);
    print("' position: ");
    print_dec(nwbytes);
    print("\n");
    nbytes = 10;
    bytes_read_from_pipe = read(pipe_id4, buffer, nbytes);
    print_fucntion_details((char *) __FUNCTION__,__LINE__);
    print(": Call Read(): pipe_id: ");
    print_dec(pipe_id4);
    print(" read_buf: '");
    print(buffer);
    print("' nbyte: ");
    print_dec(nbytes);
    print(" bytes_read_from_pipe: ");
    print_dec(bytes_read_from_pipe);
    print("\n");


}


void user_app(){
    print("User App: ");
    print(__FILE__);
    print("[");
    print_dec(__LINE__);
    print("]: ");
    print((char *)__FUNCTION__);
    print("\n");

    /////////////////////////// TESTING FORK ///////////////////////////////////
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Testing Fork\n");
    test_fork();
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Return From Testing Fork\n");

    ////////////////  TESTING PROCESS SYNCHRONIZATION //////////////////////////
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Process Sync Test\n");
    test_sem();
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Return From Process Sync Test\n");

    ///////////////////////////// TESTING PIPE /////////////////////////////////

    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Pipe Test\n");
    test_pipe();
    print_fucntion_details((char *)__FUNCTION__,__LINE__);
    print(": Return From Pipe Test\n");


}
