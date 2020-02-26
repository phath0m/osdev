#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/vnode.h>

static int dev_file_destroy(struct vnode *node);
static int dev_file_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos);
static int dev_file_stat(struct vnode *node, struct stat *stat);
static int dev_file_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos);

struct vops dev_file_ops = {
    .destroy    = dev_file_destroy,
    .read       = dev_file_read,
    .stat       = dev_file_stat,
    .write      = dev_file_write
};

struct cdev_file {
    struct vnode *      host;
    struct cdev *     device;
};

struct vnode *
cdev_file_open(struct vnode *parent, dev_t devno)
{
    struct vnode *node = vn_new(NULL, NULL, &dev_file_ops);
    struct cdev_file *file = calloc(1, sizeof(struct cdev_file));

    node->state = file;
    file->host = parent;
    file->device = cdev_from_devno(devno);

    return node;
}

static int
dev_file_destroy(struct vnode *node)
{
    struct cdev_file *file = (struct cdev_file*)node->state;

    free(file);

    return 0;
}

static int
dev_file_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct cdev_file *file = (struct cdev_file*)node->state;

    return cdev_read(file->device, buf, nbyte, pos);
}

static int
dev_file_stat(struct vnode *node, struct stat *stat)
{
    struct cdev_file *file = (struct cdev_file*)node->state;
    struct vnode *host = file->host;
    struct vops *ops = host->ops;

    if (ops->stat) {
        return ops->stat(host, stat);
    }

    return -(ENOTSUP);
}

static int
dev_file_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct cdev_file *file = (struct cdev_file*)node->state;

    return cdev_write(file->device, buf, nbyte, pos);
}
