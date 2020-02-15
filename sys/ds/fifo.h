#ifndef _DS_FIFOBUF_H
#define _DS_FIFOBUF_H

#include <sys/mutex.h>
#include <sys/types.h>

struct fifo {
    spinlock_t          lock;
    char *              buf;
    int                 head_pos;
    int                 tail_pos;
    int                 size;
    int                 buf_size;
};

#define FIFO_EMPTY(f) (f->size == 0)
#define FIFO_SIZE(f) (f->size)

struct fifo *fifo_new(size_t maxsize);
size_t fifo_read(struct fifo *fifo, void *buf, size_t nbyte);
size_t fifo_write(struct fifo *fifo, void *buf, size_t nbyte);

#endif
