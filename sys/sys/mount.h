#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <sys/vfs.h>

#define MS_RDONLY   0x01
#define MS_NOEXEC   0x08

typedef int (*fs_mount_t)(struct vfs_node *parent, struct device *dev, struct vfs_node **root);

struct fs_ops {
    fs_mount_t              mount;
};

struct filesystem {
    char *          name;
    struct fs_ops * ops;
};

struct vfs_mount {
    struct device *     device;
    struct filesystem * filesystem;
    uint64_t            flags;
};

int fs_mount(struct vfs_node *root, struct device *dev, const char *fsname, const char *path, int flags);
int fs_open(struct device *dev, struct vfs_node **root, const char *fsname, int flags);
void fs_register(char *name, struct fs_ops *ops);

#endif
