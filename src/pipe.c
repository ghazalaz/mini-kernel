#include "pipe.h"
void pipe_init(struct pipe *p) {
    p->buf = (char*)kmalloc(sizeof(char) * PIPE_BUF );
    p->len = 0;
    p->start = 0;
    p->end = 0;
}

int pipe_new() {
    int i;
    for (i = 0; i < MAX_PIPES; i++) {
        struct pipe *p = &mypipes[i];
        // Rather important stuff happening, no interrupts please!
        asm volatile("cli");
        if (!p->used) {
            pipe_init(p);
            p->used = 1;
            // Reenable interrupts.
            asm volatile("sti");
            return i;
        }
        // Reenable interrupts.
        asm volatile("sti");
    }
    return -1;
}

int pipe_read(int fildes, void *buf, int nbyte) {
    if(fildes < 0 || fildes >= MAX_PIPES) return -1;
    struct pipe *p = &mypipes[fildes];
    if(p->used == 0) return -1;
    if (nbyte < 0) { return -1; }
    if (nbyte == 0) return 0;
    // Rather important stuff happening, no interrupts please!
    asm volatile("cli");
    int nread = 0;
    for (;;) {
        if (p->len > 0) {
            int toread = MIN(p->len, nbyte);
            if(p->start < p->end) {
                memcpy(buf, p->buf + p->start, toread);
                if(p->start + toread < p->end) {
                    p->start += toread;
                } else {
                    p->start = 0;
                    p->end = 0;
                }
            } else {
                if(toread <= PIPE_BUF - p->start) {
                    memcpy(buf, p->buf + p->start , toread);
                    p->start += toread;
                    if(p->start == PIPE_BUF) p->start = 0;
                } else {
                    int read_at_end = PIPE_BUF - p->start;
                    memcpy(buf, p->buf + p->start , read_at_end);
                    memcpy(buf + read_at_end, p->buf , toread - read_at_end);
                    p->start = toread - read_at_end;
                    if(p->start == p->end) {
                        p->start = 0;
                        p->end = 0;
                    }
                }
            }
            p->len -= toread;
            nread = toread;
            break;
        }
    }
    // Reenable interrupts.
    asm volatile("sti");
    return nread;
}

int pipe_write(int fildes, const void *buf, int nbyte) {
    if(fildes < 0 || fildes >= MAX_PIPES) return -1;
    struct pipe *p = &mypipes[fildes];
    if(p->used == 0) return -1;
    if (nbyte < 0) {
        return -1;
    }
    if (nbyte == 0) return 0;
    // Rather important stuff happening, no interrupts please!
    asm volatile("cli");
    int nwritten = 0;
    if (PIPE_BUF - p->len >= nbyte) {
        int towrite = nbyte;
        if((p->start <= p->end && PIPE_BUF - p->end > towrite) ||
            (p->start > p->end && p->start - p->end > towrite)) {
            memcpy(p->buf + p->end, buf, towrite);
            p->end += towrite;
        } else if(p->start < p->end) {
            int towrite_at_end = (PIPE_BUF - p->end - 1);
            int towrite_at_start = towrite - towrite_at_end;
            memcpy(p->buf + p->end, buf, towrite_at_end);
            memcpy(p->buf, buf + towrite_at_end, towrite_at_start);
            p->end = towrite_at_start;
        }
        p->len += towrite;
        nwritten = nbyte;
    } else {
        return 0;
    }
    // Reenable interrupts.
    asm volatile("sti");
    return nwritten;
}

int pipe_free(int fildes) {
    if(fildes < 0 || fildes >= MAX_PIPES) return -1;
    struct pipe *p = &mypipes[fildes];
    if(p->used == 0) return -1;
    kfree(p->buf);
    p->used = 0;
    return fildes;
}
