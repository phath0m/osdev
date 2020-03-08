/*
 * device_file.c
 * This is responsible for connecting the character device interface to the file
 * interface. When a file with S_IFCHR is opened, cdev_to_file() will be called
 * and will lookup the device's major/minor number. If it is a registered device
 * then it will return a file struct instances that wraps the underlying device
 */
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/vnode.h>

static int dev_file_destroy(struct file *fp);
static int dev_file_getdev(struct file *fp, struct cdev **result);
static int dev_file_ioctl(struct file *fp, uint64_t request, void *arg);
static int dev_file_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset);
static int dev_file_read(struct file *fp, void *buf, size_t nbyte);
static int dev_file_stat(struct file *fp, struct stat *stat);
static int dev_file_write(struct file *fp, const void *buf, size_t nbyte);

struct fops dev_file_ops = {
    .destroy    = dev_file_destroy,
    .getdev     = dev_file_getdev,
    .ioctl      = dev_file_ioctl,
    .mmap       = dev_file_mmap,
    .read       = dev_file_read,
    .stat       = dev_file_stat,
    .write      = dev_file_write
};

struct cdev_file {
    struct vnode *      host;
    struct cdev *     device;
};

struct file *
cdev_to_file(struct vnode *host, dev_t devno)
{
    struct cdev_file *file = calloc(1, sizeof(struct cdev_file));

    file->host = host;
    file->device = cdev_from_devno(devno);

    return file_new(&dev_file_ops, file);
}

static int
dev_file_destroy(struct file *fp)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    VN_DEC_REF(file->host);

    free(file);

    return 0;
}

static int
dev_file_getdev(struct file *fp, struct cdev **result)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    *result = file->device;

    return 0;
}

static int
dev_file_ioctl(struct file *fp, uint64_t request, void *arg)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    return cdev_ioctl(file->device, request, (uintptr_t)arg);
}

static int
dev_file_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    return cdev_mmap(file->device, addr, size, prot, offset);
}

static int
dev_file_read(struct file *fp, void *buf, size_t nbyte)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    return cdev_read(file->device, buf, nbyte, 0);
}

static int
dev_file_stat(struct file *fp, struct stat *stat)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;
    struct vnode *host = file->host;
    struct vops *ops = host->ops;

    if (ops->stat) {
        return ops->stat(host, stat);
    }

    return -(ENOTSUP);
}

static int
dev_file_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct cdev_file *file = (struct cdev_file*)fp->state;

    return cdev_write(file->device, buf, nbyte, 0);
}
