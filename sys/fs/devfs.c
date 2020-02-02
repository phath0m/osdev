#include <ds/list.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vfs.h>


static int devfs_destroy(struct vfs_node *node);
static int devfs_ioctl(struct vfs_node *node, uint64_t mode, void *arg);

static int devfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name);
static int devfs_mount(struct vfs_node *parent, struct device *dev, struct vfs_node **root);
static int defops_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int defops_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int defops_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence);
static int defops_stat(struct vfs_node *node, struct stat *stat);
static int defops_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops devfs_file_ops = {
    .destroy    = devfs_destroy,
    .ioctl      = devfs_ioctl,
    .lookup     = devfs_lookup,
    .read       = defops_read,
    .readdirent = defops_readdirent,
    .seek       = defops_seek,
    .stat       = defops_stat,
    .write      = defops_write
};

struct fs_ops devfs_ops = {
    .mount      = devfs_mount,
};

/*
 * defined in kern/device.c
 */
extern struct list device_list;

static int
devfs_destroy(struct vfs_node *node)
{
    return 0;
}

static int
devfs_ioctl(struct vfs_node *node, uint64_t mode, void *arg)
{
    struct device *dev = (struct device*)node->state;

    if (dev) {
        return device_ioctl(dev, mode, (uintptr_t)arg);
    }

    return 0;
}

static int
devfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name)
{
    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct device *dev;
    
    int res = -1;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (strcmp(name, dev->name) == 0) {
            struct vfs_node *node = vfs_node_new(parent, parent->device, &devfs_file_ops);
            node->device = dev;
            node->inode = (ino_t)dev;
            node->gid = 0;
            node->uid = 0;
            node->state = (void*)dev;

            *result = node;

            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

static int
devfs_mount(struct vfs_node *parent, struct device *dev, struct vfs_node **root)
{
    struct vfs_node *node = vfs_node_new(parent, dev, &devfs_file_ops);

    node->inode = 0;
    node->gid = 0;
    node->uid = 0;
    node->mode = 0755 | S_IFDIR;

    *root = node;

    return 0;
}

static int
defops_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{

    struct device *dev = (struct device*)node->state;

    if (dev && node->inode != 0) {
        return device_read(dev, buf, nbyte, pos);
    }

    return -1;
}

static int
defops_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry)
{
    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct device *dev;

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
defops_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence)
{
    if (whence == SEEK_SET) {
        *cur_pos = off;
        return *cur_pos;
    }

    return -(ESPIPE);
}

static int
defops_stat(struct vfs_node *node, struct stat *stat)
{
    if (node->inode == 0) {
        stat->st_mode = 0755 | S_IFDIR;
        return 0;
    }

    struct device *dev = (struct device*)node->state;

    if (!dev) {
        return -1;    
    }

    stat->st_mode = dev->mode | S_IFCHR;

    return 0;
}

static int
defops_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct device *dev = (struct device*)node->state;

    if (!dev) {
        return -1;
    }

    return device_write(dev, buf, nbyte, pos);
}

__attribute__((constructor))
void
_init_devfs()
{
    fs_register("devfs", &devfs_ops);
}
