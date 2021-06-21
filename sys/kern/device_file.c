/*
 * device_file.c - device file implementation
 *
 * This is responsible for connecting the character device interface to the file
 * interface. When a file with S_IFCHR is opened, cdev_to_file() will be called
 * and will lookup the device's major/minor number. If it is a registered device
 * then it will return a file struct instances that wraps the underlying device
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
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>

static int dev_file_destroy(struct file *);
static int dev_file_getdev(struct file *, struct cdev **);
static int dev_file_ioctl(struct file *, uint64_t, void *);
static int dev_file_mmap(struct file *, uintptr_t, size_t, int, off_t);
static int dev_file_read(struct file *, void *, size_t);
static int dev_file_seek(struct file *, off_t *, off_t, int);
static int dev_file_stat(struct file *, struct stat *);
static int dev_file_write(struct file *, const void *, size_t);

struct fops dev_file_ops = {
    .destroy    = dev_file_destroy,
    .getdev     = dev_file_getdev,
    .ioctl      = dev_file_ioctl,
    .mmap       = dev_file_mmap,
    .read       = dev_file_read,
    .seek       = dev_file_seek,
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
    struct cdev_file *file;
    
    file = calloc(1, sizeof(struct cdev_file));

    file->host = host;
    file->device = cdev_from_devno(devno);

    return file_new(&dev_file_ops, file);
}

static int
dev_file_destroy(struct file *fp)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    VN_DEC_REF(file->host);

    free(file);

    return 0;
}

static int
dev_file_getdev(struct file *fp, struct cdev **result)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    *result = file->device;

    return 0;
}

static int
dev_file_ioctl(struct file *fp, uint64_t request, void *arg)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    return cdev_ioctl(file->device, request, (uintptr_t)arg);
}

static int
dev_file_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    return cdev_mmap(file->device, addr, size, prot, offset);
}

static int
dev_file_read(struct file *fp, void *buf, size_t nbyte)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    return cdev_read(file->device, buf, nbyte, fp->position);
}


static int
dev_file_seek(struct file *file, off_t *cur_pos, off_t off, int whence)
{
    off_t new_pos;

    switch (whence) {
        case SEEK_CUR:
            new_pos = *cur_pos + off;
            break;
        case SEEK_SET:
            new_pos = off;
            break;
    }

    if (new_pos < 0) {
        return -(ESPIPE);
    }

    *cur_pos = new_pos;

    return new_pos;
}

static int
dev_file_stat(struct file *fp, struct stat *stat)
{
    struct cdev_file *file;
    struct vnode *host;
    struct vops *ops;

    file = fp->state;
    host = file->host;
    ops = host->ops;

    if (ops->stat) {
        return ops->stat(host, stat);
    }

    return -(ENOTSUP);
}

static int
dev_file_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct cdev_file *file;
    
    file = (struct cdev_file*)fp->state;

    return cdev_write(file->device, buf, nbyte, fp->position);
}