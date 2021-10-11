/*
 * devfs.c - Implementation of the device filesystem (/dev)
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
#include <ds/list.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>

// delete me
#include <sys/systm.h>

static int devfs_destroy(struct vnode *);
static int devfs_ioctl(struct vnode *, uint64_t, void *);

static int devn_lookup(struct vnode *, struct vnode **, const char *);
static int devfs_mmap(struct vnode *, uintptr_t, size_t, int, off_t);
static int devfs_mount(struct vnode *, struct file *, struct vnode **);
static int devfs_read(struct vnode *, void *, size_t, uint64_t);
static int devfs_readdirent(struct vnode *, struct dirent *, uint64_t);
static int devfs_seek(struct vnode *, off_t *, off_t, int);
static int devfs_stat(struct vnode *, struct stat *);
static int devfs_write(struct vnode *, const void *, size_t, uint64_t);

struct vops devfs_file_ops = {
    .destroy    = devfs_destroy,
    .ioctl      = devfs_ioctl,
    .lookup     = devn_lookup,
    .mmap       = devfs_mmap,
    .read       = devfs_read,
    .readdirent = devfs_readdirent,
    .seek       = devfs_seek,
    .stat       = devfs_stat,
    .write      = devfs_write
};

struct fs_ops devfs_ops = {
    .mount      = devfs_mount,
};

/*
 * defined in kern/device.c
 */
extern struct list device_list;

static int
devfs_destroy(struct vnode *node)
{
    return 0;
}

static int
devfs_ioctl(struct vnode *node, uint64_t mode, void *arg)
{
    struct cdev *dev;

    dev = (struct cdev*)node->state;
    
    if (dev) {
        return CDEVOPS_IOCTL(dev, mode, (uintptr_t)arg);
    }

    return 0;
}

static int
devn_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    int res;
    list_iter_t iter;

    struct cdev *dev;
    struct vnode *node;

    res = -1;

    list_get_iter(&device_list, &iter);

    while (iter_move_next(&iter, (void**)&dev)) {
        if (strcmp(name, dev->name) == 0) {
            node = vn_new(parent, parent->device, &devfs_file_ops);
            node->device = dev;
            node->inode = (ino_t)dev;
            node->gid = 0;
            node->uid = dev->uid;
            node->state = (void*)dev;
            node->devno = makedev(dev->majorno, dev->minorno);
            node->mode = dev->mode | S_IFCHR; 
            *result = node;

            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

int
devfs_mmap(struct vnode *node, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct cdev *dev;

    dev = (struct cdev*)node->state;

    if (dev && node->inode != 0) {
        panic("mmap");
        return CDEVOPS_MMAP(dev, addr, size, prot, offset);
    }

    return -(ENOTSUP); 
}

static int
devfs_mount(struct vnode *parent, struct file *dev_fp, struct vnode **root)
{
    struct vnode *node;

    node = vn_new(parent, NULL, &devfs_file_ops);

    node->inode = 0;
    node->gid = 0;
    node->uid = 0;
    node->mode = 0755 | S_IFDIR;

    *root = node;

    return 0;
}

static int
devfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{

    struct cdev *dev;

    dev = (struct cdev*)node->state;

    if (dev && node->inode != 0) {
        return CDEVOPS_READ(dev, buf, nbyte, pos);
    }

    return -1;
}

static int
devfs_readdirent(struct vnode *node, struct dirent *dirent, uint64_t entry)
{
    int i;
    int res;
    list_iter_t iter;
    struct cdev *dev;

    res = -1;

    list_get_iter(&device_list, &iter);

    for (i = 0; iter_move_next(&iter, (void**)&dev); i++) {
        if (i == entry) {
            strncpy(dirent->name, dev->name, PATH_MAX);
            dirent->type = DT_CHR;
            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

static int
devfs_seek(struct vnode *node, off_t *cur_pos, off_t off, int whence)
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
devfs_stat(struct vnode *node, struct stat *stat)
{
    struct cdev *dev;

    memset(stat, 0, sizeof(struct stat));

    if (node->inode == 0) {
        stat->st_uid = 0;
        stat->st_mode = 0755 | S_IFDIR;
        return 0;
    }

    dev = (struct cdev*)node->state;

    if (!dev) {
        return -1;    
    }

    stat->st_mode = dev->mode | S_IFCHR;
    stat->st_uid = dev->uid;
    stat->st_dev = makedev(dev->majorno, dev->minorno);

    return 0;
}

static int
devfs_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct cdev *dev;

    dev = (struct cdev*)node->state;

    if (!dev) {
        return -1;
    }

    return CDEVOPS_WRITE(dev, buf, nbyte, pos);
}

void
devfs_init()
{
    fs_register("devfs", &devfs_ops);
}
