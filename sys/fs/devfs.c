#include <rtl/list.h>
#include <rtl/string.h>
#include <rtl/types.h>
#include <sys/limits.h>
#include <sys/vfs.h>


static int devfs_close(struct vfs_node *node);
static int devfs_open(struct fs_mount *mount, struct vfs_node *node, const char *path);
static int devfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int devfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int devfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct fs_ops devfs_ops = {
    .close      = devfs_close,
    .open       = devfs_open,
    .read       = devfs_read,
    .readdirent = devfs_readdirent,
    .write      = devfs_write
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
devfs_open(struct fs_mount *mount, struct vfs_node *node, const char *path)
{
    return 0;
}

static int
devfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    return 0;
}

static int
devfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry)
{
    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct device *dev;

    int i = 0;
    int ret = -1;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (i == entry) {
            strncpy(dirent->name, dev->name, PATH_MAX);

            ret = 0;

            break;
        }

        i++;
    }

    iter_close(&iter);

    return ret;
}

static int
devfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    return 0;
}

__attribute__((constructor)) static void
devfs_init()
{
    register_filesystem("devfs", &devfs_ops);
}
