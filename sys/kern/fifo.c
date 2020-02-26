/*
 * fifo.c - named pipe implementation
 */
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/pipe.h>
#include <sys/systm.h>
#include <sys/vnode.h>

static int fifo_close(struct vnode *node, struct file *fp);
static int fifo_destroy(struct vnode *node);
static int fifo_duplicate(struct vnode *node, struct file *newfile);
static int fifo_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos);
static int fifo_stat(struct vnode *node, struct stat *stat);
static int fifo_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos);

struct vops fifo_ops = {
    .close      = fifo_close,
    .destroy    = fifo_destroy,
    .duplicate  = fifo_duplicate,
    .read       = fifo_read,
    .stat       = fifo_stat,
    .write      = fifo_write
};

struct fifo {
    struct file *       pipe[2];
    struct vnode *   host;
    int                 write_refs;
    int                 read_refs;
};

static struct vnode *
fifo_open_write(struct vnode *parent)
{
    while (LIST_SIZE(&parent->un.fifo_readers) == 0) {
        thread_yield();
    }

    struct vnode *reader = list_peek_back(&parent->un.fifo_readers);

    struct fifo *fifo = (struct fifo*)reader->state;

    fifo->write_refs++;

    return reader;
}

static struct vnode *
fifo_open_read(struct vnode *parent)
{
    struct fifo *fifo = calloc(1, sizeof(struct fifo));

    fifo->read_refs = 1;
    fifo->host = parent;

    create_pipe(fifo->pipe);

    struct vnode *node = vn_new(NULL, NULL, &fifo_ops);

    node->state = fifo;

    list_append(&parent->un.fifo_readers, node);

    return node;
}

struct vnode *
fifo_open(struct vnode *parent, mode_t mode)
{
    if ((mode & O_WRONLY)) {
        return fifo_open_write(parent);
    }

    return fifo_open_read(parent);
}

static int
fifo_close(struct vnode *node, struct file *fp)
{
    struct fifo *fifo = (struct fifo*)node->state;
    struct vnode *host = fifo->host;

    if ((fp->flags & O_WRONLY)) {
        fifo->write_refs--; 
    } else {
        fifo->read_refs--;
    }

    if (fifo->write_refs == 0) {
        vops_close(fifo->pipe[1]);
        fifo->write_refs--;
    }

    if (fifo->read_refs == 0) {
        vops_close(fifo->pipe[0]);
        list_remove(&host->un.fifo_readers, node);
        fifo->read_refs--;
    }

    return 0;
}

static int
fifo_destroy(struct vnode *node)
{
    return 0;
}

static int
fifo_duplicate(struct vnode *node, struct file *newfile)
{
    struct fifo *fifo = (struct fifo*)node->state;

    if (newfile->flags & O_WRONLY) {
        fifo->write_refs++;
    } else {
        fifo->read_refs++;
    }

    return 0;
}

static int
fifo_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct fifo *fifo = (struct fifo*)node->state;

    return vops_read(fifo->pipe[0], buf, nbyte);
}

static int
fifo_stat(struct vnode *node, struct stat *stat)
{
    struct fifo *fifo = (struct fifo*)node->state;
    struct vnode *host = fifo->host;
    struct vops *ops = host->ops;

    if (ops->stat) {
        return ops->stat(host, stat);
    }

    return -(ENOTSUP);
}

static int
fifo_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct fifo *fifo = (struct fifo*)node->state;

    return vops_write(fifo->pipe[1], buf, nbyte);
}
