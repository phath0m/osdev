/*
 * pipe.c - anonymous pipe implementation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/types.h>
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

static int pipe_close(struct file *);
static int pipe_destroy(struct file *);
static int pipe_duplicate(struct file *);
static int pipe_read(struct file *, void *, size_t);
static int pipe_write(struct file *, const void *, size_t);

struct fops pipe_ops = {
    .close      = pipe_close,
    .destroy    = pipe_destroy,
    .duplicate  = pipe_duplicate,
    .read       = pipe_read,
    .write      = pipe_write
};

struct pipe *
pipe_new()
{
    struct pipe *pipe;
    
    pipe = calloc(1, sizeof(struct pipe));
    pipe->buf = malloc(4096);
    pipe->buf_size = 4096;
    pipe->ref_count = 2;
    pipe->write_refs = 1;
    pipe->read_refs = 1;

    return pipe;
}

void
create_pipe(struct file **pipes)
{
    struct pipe *pipe;
    struct file *read_end;
    struct file *write_end;
 
    pipe = pipe_new();
    read_end = file_new(&pipe_ops, NULL);
    write_end = file_new(&pipe_ops, NULL);
  
    read_end->state = pipe; 
    read_end->flags = O_RDONLY;
    write_end->state = pipe;
    write_end->flags = O_WRONLY;

    pipes[0] = read_end;
    pipes[1] = write_end;
}

static int
pipe_close(struct file *fp)
{
    struct pipe *pipe;
    
    pipe = fp->state;

    if (fp->flags & O_WRONLY) {
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
pipe_destroy(struct file *fp)
{
    struct pipe *pipe;
    
    pipe = fp->state;

    free(pipe->buf);
    free(pipe);

    return 0;
}

static int
pipe_duplicate(struct file *newfile)
{
    struct pipe *pipe;
    
    pipe = newfile->state;

    if (newfile->flags & O_WRONLY) {
        pipe->write_refs++;
    } else {
        pipe->read_refs++;
    }

    return 0;
}

static int
pipe_read(struct file *fp, void *buf, size_t nbyte)
{
    extern struct thread *sched_curr_thread;

    int i;
    uint8_t *buf8;
    struct pipe *pipe;
    
    pipe = fp->state;

    if (!pipe) {
        return -1;
    }

    buf8 = buf;

    while (pipe->size == 0 && !pipe->write_closed) {
        thread_yield();

        if (sched_curr_thread->exit_requested) {
            return -(EINTR);
        }
    }

    if (pipe->write_closed && pipe->size == 0) {
        return -(EPIPE);
    }

    spinlock_lock(&pipe->lock);

    if (nbyte > pipe->size) {
        nbyte = pipe->size;
    }

    for (i = 0; i < nbyte; i++) {
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
pipe_write(struct file *fp, const void *buf, size_t nbyte)
{
    extern struct thread *sched_curr_thread;
        
    int i;
    uint8_t const *buf8;
    struct pipe *pipe;
    
    pipe = fp->state;

    if (!pipe) {
        return -1;
    }

    while (pipe->size != 0 && !pipe->read_closed) {
        if (sched_curr_thread->exit_requested) {
            return -(EINTR);
        }
    }

    if (pipe->read_closed) {
        return -(EPIPE);
    }

    spinlock_lock(&pipe->lock);

    buf8 = buf;

    for (i = 0; i < nbyte; i++) {
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
