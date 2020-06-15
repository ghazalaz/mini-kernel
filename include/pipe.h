#ifndef MYPIPE_H
#define MYPIPE_H

#include "common.h"
#include "kheap.h"

#define PIPE_BUF 1024
#define MAX_PIPES 256

struct pipe {
    void *buf;
    int len;
    int used;
    int start;
    int end;
};
struct pipe mypipes[MAX_PIPES];
void pipe_init(struct pipe *);
int pipe_new();
int pipe_read(int fildes, void *buf, int nbyte);
int pipe_write(int fildes, const void *buf, int nbyte);
int pipe_free(int fildes);
#endif
