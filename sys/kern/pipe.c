#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/wait.h>

// remove me
#include <stdio.h>

struct pipe {
    struct wait_queue   read_queue;
    struct wait_queue   write_queue;
    spinlock_t          lock;
    char *              buf;
    size_t              pending;
    off_t               pos;
    size_t              len;
};

static int pipe_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int pipe_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops pipe_ops = {
    .read = pipe_read,
    .write = pipe_write
};

struct pipe *
pipe_new()
{
    struct pipe *pipe = (struct pipe*)calloc(0, sizeof(struct pipe));
    pipe->pos = 0;
    pipe->pending = 0;
    pipe->buf = (char*)malloc(4096);
    pipe->len = 4096;

    return pipe;
}

void
create_pipe(struct file **pipes)
{
    struct vfs_node *node = vfs_node_new(NULL, &pipe_ops);
    struct file *read_end = file_new(node);
    struct file *write_end = file_new(node);
   
    node->state = pipe_new();

    read_end->flags = O_RDONLY;
    write_end->flags = O_WRONLY;

    pipes[0] = read_end;
    pipes[1] = write_end;
}

static int
pipe_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct pipe *pipe = (struct pipe*)node->state;

    uint8_t *buf8 = (uint8_t*)buf;

    while (pipe->pending == 0) {
        sched_yield();
    }

    spinlock_lock(&pipe->lock);

    int npending = pipe->pending;

    int i;

    for (i = 0; i < npending; i++) {
        buf8[i] = (uint8_t)pipe->buf[i];
    }

    pipe->pending = 0;

    spinlock_unlock(&pipe->lock);

    return i;
}


static int
pipe_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct pipe *pipe = (struct pipe*)node->state;

    uint8_t *buf8 = (uint8_t*)buf;

    while (pipe->pending != 0) {
        sched_yield();
    }

    spinlock_lock(&pipe->lock);

    for (int i = 0; i < nbyte; i++) {
        pipe->buf[i] = buf8[i];
    }

    pipe->pending = nbyte;

    spinlock_unlock(&pipe->lock);

    wq_pulse(&pipe->write_queue);

    return nbyte;
}
