#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/wait.h>

struct pipe {
    struct wait_queue   read_queue;
    struct wait_queue   write_queue;
    bool                closed;
    bool                write_closed;
    bool                read_closed;
    int                 ref_count;
    spinlock_t          lock;
    char *              buf;
    int                 head_pos;
    int                 tail_pos;
    int                 size;
    int                 buf_size;
    int                 read_refs;
    int                 write_refs;
};

static int pipe_close(struct vfs_node *node, struct file *fp);
static int pipe_destroy(struct vfs_node *node);
static int pipe_duplicate(struct vfs_node *node, struct file *newfile);
static int pipe_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int pipe_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops pipe_ops = {
    .close      = pipe_close,
    .destroy    = pipe_destroy,
    .duplicate  = pipe_duplicate,
    .read       = pipe_read,
    .write      = pipe_write
};

struct pipe *
pipe_new()
{
    struct pipe *pipe = (struct pipe*)calloc(0, sizeof(struct pipe));
    pipe->buf = (char*)malloc(4096);
    pipe->buf_size = 4096;
    pipe->ref_count = 2;
    pipe->write_refs = 1;
    pipe->read_refs = 1;
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
pipe_close(struct vfs_node *node, struct file *file)
{
    struct pipe *pipe = (struct pipe*)node->state;

    if (file->flags & O_WRONLY) {
        pipe->write_refs--;
    } else {
        pipe->read_refs--;
    }

    if (pipe->read_refs == 0) {
        pipe->read_closed = true;
    }

    if (pipe->write_refs == 0) {
        pipe->write_closed = true;
    }
    return 0;
}

static int
pipe_destroy(struct vfs_node *node)
{
    struct pipe *pipe = (struct pipe*)node->state;

    free(pipe->buf);
    free(pipe);

    return 0;
}

static int
pipe_duplicate(struct vfs_node *node, struct file *newfile)
{
    struct pipe *pipe = (struct pipe*)node->state;

    if (newfile->flags & O_WRONLY) {
        pipe->write_refs++;
    } else {
        pipe->read_refs++;
    }

    return 0;
}

static int
pipe_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct pipe *pipe = (struct pipe*)node->state;

    if (!pipe) {
        return -1;
    }

    uint8_t *buf8 = (uint8_t*)buf;

    while (pipe->size == 0 && !pipe->write_closed) {
        thread_yield();
    }

    spinlock_lock(&pipe->lock);

    if (nbyte > pipe->size) {
        nbyte = pipe->size;
    }

    for (int i = 0; i < nbyte; i++) {
        buf8[i] = pipe->buf[i % pipe->buf_size + pipe->head_pos];
    }

    pipe->size -= nbyte;
    pipe->head_pos += nbyte;

    if (pipe->size == 0) {
        pipe->head_pos = 0;
        pipe->tail_pos = 0;
    }
 
    spinlock_unlock(&pipe->lock);

    return nbyte;
}


static int
pipe_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct pipe *pipe = (struct pipe*)node->state;

    if (!pipe) {
        return -1;
    }

    while (pipe->size != 0 && !pipe->read_closed) {
        //thread_yield();
    }

    spinlock_lock(&pipe->lock);

    uint8_t *buf8 = (uint8_t*)buf;

    for (int i = 0; i < nbyte; i++) {
        pipe->buf[(i + pipe->tail_pos) % pipe->buf_size] = buf8[i];
    }

    pipe->tail_pos += nbyte;
    pipe->size += nbyte;

    spinlock_unlock(&pipe->lock);

    // Note: wait queue is broken, however, this should eventually use the wait queue
    // to put the thread to sleep while waiting for the pipe to fill/drain
    //wq_pulse(&pipe->write_queue);

    return nbyte;
}
