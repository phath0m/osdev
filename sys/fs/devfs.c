#include <stdlib.h>
#include <string.h>
#include <ds/list.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/types.h>
#include <sys/vfs.h>

static int devfs_close(struct vfs_node *node);
static int devfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name);
static int devfs_mount(struct device *dev, struct vfs_node **root);
static int devfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int devfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int devfs_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence);
static int devfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops devfs_file_ops = {
    .close      = devfs_close,
    .lookup     = devfs_lookup,
    .read       = devfs_read,
    .readdirent = devfs_readdirent,
    .seek       = devfs_seek,
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
devfs_close(struct vfs_node *node)
{
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
            struct vfs_node *node = vfs_node_new(parent->device, &devfs_file_ops);
            node->inode = (ino_t)dev;
            node->group = 0;
            node->owner = 0;
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
devfs_mount(struct device *dev, struct vfs_node **root)
{
    struct vfs_node *node = vfs_node_new(dev, &devfs_file_ops);

    node->inode = 0;
    node->group = 0;
    node->owner = 0;

    *root = node;

    return 0;
}

static int
devfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{

    struct device *dev = (struct device*)node->state;

    if (dev && node->inode != 0) {
        return device_read(dev, buf, nbyte, pos);
    }

    return -1;
}

static int
devfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry)
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
devfs_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence)
{
    if (whence == SEEK_SET) {
        *cur_pos = off;
        return 0;
    }

    return ESPIPE;
}

static int
devfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct device *dev = (struct device*)node->state;

    if (dev) {
        return device_write(dev, buf, nbyte, pos);
    }

    return 0;
}

__attribute__((constructor)) static void
devfs_init()
{
    register_filesystem("devfs", &devfs_ops);
}
