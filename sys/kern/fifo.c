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

static int fifo_close(struct file *fp);
static int fifo_destroy(struct file *fp);
static int fifo_duplicate(struct file *fp);
static int fifo_read(struct file *fp, void *buf, size_t nbyte);
static int fifo_stat(struct file *fp, struct stat *stat);
static int fifo_write(struct file *fp, const void *buf, size_t nbyte);

struct fops fifo_ops = {
    .close      = fifo_close,
    .destroy    = fifo_destroy,
    .duplicate  = fifo_duplicate,
    .read       = fifo_read,
    .stat       = fifo_stat,
    .write      = fifo_write
};

struct fifo {
    struct file *       pipe[2];
    struct vnode *      host;
    int                 write_refs;
    int                 read_refs;
};

static struct file *
fifo_open_write(struct vnode *parent)
{
    while (LIST_SIZE(&parent->un.fifo_readers) == 0) {
        thread_yield();
    }

    struct fifo *reader = list_peek_back(&parent->un.fifo_readers);

    reader->write_refs++;

    return file_new(&fifo_ops, reader);
}

static struct file *
fifo_open_read(struct vnode *host)
{
    struct fifo *fifo = calloc(1, sizeof(struct fifo));

    fifo->read_refs = 1;
    fifo->host = host;

    create_pipe(fifo->pipe);

    struct file *fp = file_new(&fifo_ops, fifo);

    fp->state = fifo;

    list_append(&host->un.fifo_readers, fifo);

    return fp;
}

struct file *
fifo_to_file(struct vnode *parent, mode_t mode)
{
    if ((mode & O_WRONLY)) {
        return fifo_open_write(parent);
    }

    return fifo_open_read(parent);
}

static int
fifo_close(struct file *fp)
{
    struct fifo *fifo = (struct fifo*)fp->state;
    struct vnode *host = fifo->host;

    if ((fp->flags & O_WRONLY)) {
        fifo->write_refs--; 
    } else {
        fifo->read_refs--;
    }

    if (fifo->write_refs == 0) {
        fop_close(fifo->pipe[1]);
        fifo->write_refs--;
    }

    if (fifo->read_refs == 0) {
        fop_close(fifo->pipe[0]);
        list_remove(&host->un.fifo_readers, fifo);
        fifo->read_refs--;
    }

    return 0;
}

static int
fifo_destroy(struct file *fp)
{
    return 0;
}

static int
fifo_duplicate(struct file *fp)
{
    struct fifo *fifo = (struct fifo*)fp->state;

    if (fp->flags & O_WRONLY) {
        fifo->write_refs++;
    } else {
        fifo->read_refs++;
    }

    return 0;
}

static int
fifo_read(struct file *fp, void *buf, size_t nbyte)
{
    struct fifo *fifo = (struct fifo*)fp->state;

    return fop_read(fifo->pipe[0], buf, nbyte);
}

static int
fifo_stat(struct file *fp, struct stat *stat)
{
    struct fifo *fifo = (struct fifo*)fp->state;
    struct vnode *host = fifo->host;
    struct vops *ops = host->ops;

    if (ops->stat) {
        return ops->stat(host, stat);
    }

    return -(ENOTSUP);
}

static int
fifo_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct fifo *fifo = (struct fifo*)fp->state;

    return fop_write(fifo->pipe[1], buf, nbyte);
}
