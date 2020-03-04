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

static int devfs_destroy(struct vnode *node);
static int devfs_ioctl(struct vnode *node, uint64_t mode, void *arg);

static int devn_lookup(struct vnode *parent, struct vnode **result, const char *name);
static int devfs_mmap(struct vnode *node, uintptr_t addr, size_t size, int prot, off_t offset);
static int devfs_mount(struct vnode *parent, struct cdev *dev, struct vnode **root);
static int devfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos);
static int devfs_readdirent(struct vnode *node, struct dirent *dirent, uint64_t entry);
static int devfs_seek(struct vnode *node, off_t *cur_pos, off_t off, int whence);
static int devfs_stat(struct vnode *node, struct stat *stat);
static int devfs_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos);

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
    struct cdev *dev = (struct cdev*)node->state;
    
    if (dev) {
        return cdev_ioctl(dev, mode, (uintptr_t)arg);
    }

    return 0;
}

static int
devn_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct cdev *dev;
    
    int res = -1;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (strcmp(name, dev->name) == 0) {
            struct vnode *node = vn_new(parent, parent->device, &devfs_file_ops);
            node->device = dev;
            node->inode = (ino_t)dev;
            node->gid = 0;
            node->uid = 0;
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
    struct cdev *dev = (struct cdev*)node->state;

    if (dev && node->inode != 0) {
        panic("mmap");
        return cdev_mmap(dev, addr, size, prot, offset);
    }

    return -(ENOTSUP); 
}

static int
devfs_mount(struct vnode *parent, struct cdev *dev, struct vnode **root)
{
    struct vnode *node = vn_new(parent, dev, &devfs_file_ops);

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

    struct cdev *dev = (struct cdev*)node->state;

    if (dev && node->inode != 0) {
        return cdev_read(dev, buf, nbyte, pos);
    }

    return -1;
}

static int
devfs_readdirent(struct vnode *node, struct dirent *dirent, uint64_t entry)
{
    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct cdev *dev;

    int res = -1;

    for (int i = 0; iter_move_next(&iter, (void**)&dev); i++) {
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
    if (whence == SEEK_SET) {
        *cur_pos = off;
        return *cur_pos;
    }

    return -(ESPIPE);
}

static int
devfs_stat(struct vnode *node, struct stat *stat)
{
    if (node->inode == 0) {
        stat->st_mode = 0755 | S_IFDIR;
        return 0;
    }

    struct cdev *dev = (struct cdev*)node->state;

    if (!dev) {
        return -1;    
    }

    stat->st_mode = dev->mode | S_IFCHR;

    return 0;
}

static int
devfs_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct cdev *dev = (struct cdev*)node->state;

    if (!dev) {
        return -1;
    }

    return cdev_write(dev, buf, nbyte, pos);
}

void
devfs_init()
{
    fs_register("devfs", &devfs_ops);
}
