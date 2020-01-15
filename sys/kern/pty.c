#include <stdlib.h>
#include <string.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/vfs.h>


static int ptm_close(struct vfs_node *node);
static int ptm_destroy(struct vfs_node *node);
static int ptm_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int ptm_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);
static int pts_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);
static int pts_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

struct file_ops ptm_ops = {
    .close      = ptm_close,
    .destroy    = ptm_destroy,
    .read       = ptm_read,
    .write      = ptm_write
};


struct pty {
    struct file *   input_pipe[2];
    struct file *   output_pipe[2];
    struct device * slave;
};

static int pty_counter = 0;

static struct device *
mkpty_slave(struct pty *pty)
{
    struct device *pts_dev = calloc(1, sizeof(struct device) + 14);
    char *name = (char*)&pts_dev[1];

    sprintf(name, "pt%d", pty_counter++);

    pts_dev->name = name;

    pts_dev->mode = 0600;
    pts_dev->state = pty;
    pts_dev->read = pts_read;
    pts_dev->write = pts_write;

    device_register(pts_dev);

    return pts_dev;
}

struct file *
mkpty()
{
    struct pty *pty = calloc(1, sizeof(struct pty));

    create_pipe(pty->input_pipe);
    create_pipe(pty->output_pipe);

    pty->slave = mkpty_slave(pty);

    struct vfs_node *node = vfs_node_new(NULL, &ptm_ops);

    node->state = pty;

    struct file *res = file_new(node);

    res->flags = O_RDWR;

    return res;
}


static
int ptm_close(struct vfs_node *node)
{
    return 0;
}

static int
ptm_destroy(struct vfs_node *node)
{
    return 0;
}

static
int ptm_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)node->state;

    return fops_read(pty->output_pipe[0], buf, nbyte);
}

static int
ptm_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)node->state;

    return fops_write(pty->input_pipe[1], buf, nbyte);
}

static int
pts_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    return fops_read(pty->input_pipe[0], buf, nbyte);
}

static int
pts_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    return fops_write(pty->output_pipe[1], buf, nbyte);
}
