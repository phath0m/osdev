#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/vfs.h>


static int fifo_close(struct vfs_node *node, struct file *fp);
static int fifo_destroy(struct vfs_node *node);
static int fifo_duplicate(struct vfs_node *node, struct file *newfile);
static int fifo_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int fifo_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops fifo_ops = {
    .close      = fifo_close,
    .destroy    = fifo_destroy,
    .duplicate  = fifo_duplicate,
    .read       = fifo_read,
    .write      = fifo_write
};

struct fifo {
    struct file *       pipe[2];
    struct vfs_node *   host;
    int                 write_refs;
    int                 read_refs;
};

static struct vfs_node *
fifo_open_write(struct vfs_node *parent)
{
    while (LIST_SIZE(&parent->fifo_readers) == 0) {
        thread_yield();
    }

    struct vfs_node *reader = list_peek_back(&parent->fifo_readers);

    struct fifo *fifo = (struct fifo*)reader->state;

    fifo->write_refs++;

    return reader;
}

static struct vfs_node *
fifo_open_read(struct vfs_node *parent)
{
    struct fifo *fifo = calloc(1, sizeof(struct fifo));

    fifo->read_refs = 1;
    fifo->host = parent;

    create_pipe(fifo->pipe);

    struct vfs_node *node = vfs_node_new(NULL, NULL, &fifo_ops);

    node->state = fifo;

    list_append(&parent->fifo_readers, node);

    return node;
}

struct vfs_node *
fifo_open(struct vfs_node *parent, mode_t mode)
{
    if ((mode & O_WRONLY)) {
        return fifo_open_write(parent);
    }

    return fifo_open_read(parent);
}

static int
fifo_close(struct vfs_node *node, struct file *fp)
{
    struct fifo *fifo = (struct fifo*)node->state;
    struct vfs_node *host = fifo->host;

    if ((fp->flags & O_WRONLY)) {
        fifo->write_refs--; 
    } else {
        fifo->read_refs--;
    }

    if (fifo->write_refs == 0) {
        fops_close(fifo->pipe[1]);
        fifo->write_refs--;
    }

    if (fifo->read_refs == 0) {
        fops_close(fifo->pipe[0]);
        list_remove(&host->fifo_readers, node);
        fifo->read_refs--;
    }

    return 0;
}

static int
fifo_destroy(struct vfs_node *node)
{
    return 0;
}

static int
fifo_duplicate(struct vfs_node *node, struct file *newfile)
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
fifo_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct fifo *fifo = (struct fifo*)node->state;

    return fops_read(fifo->pipe[0], buf, nbyte);
}

static int
fifo_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct fifo *fifo = (struct fifo*)node->state;

    return fops_write(fifo->pipe[1], buf, nbyte);
}
