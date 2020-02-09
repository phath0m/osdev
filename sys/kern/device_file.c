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

struct device_file {
    struct vnode *      host;
    struct device *     device;
};

struct vnode *
device_file_open(struct vnode *parent, dev_t devno)
{
    struct vnode *node = vn_new(NULL, NULL, &dev_file_ops);
    struct device_file *file = calloc(1, sizeof(struct device_file));

    file->host = parent;
    file->device = device_from_devno(devno);

    return node;
}

static int
dev_file_destroy(struct vnode *node)
{
    struct device_file *file = (struct device_file*)node->state;

    free(file);

    return 0;
}

static int
dev_file_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct device_file *file = (struct device_file*)node->state;

    return device_read(file->device, buf, nbyte, pos);
}

static int
dev_file_stat(struct vnode *node, struct stat *stat)
{
    struct device_file *file = (struct device_file*)node->state;
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
    struct device_file *file = (struct device_file*)node->state;

    return device_write(file->device, buf, nbyte, pos);
}
