#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <sys/vnode.h>

#define MS_RDONLY   0x01
#define MS_NOEXEC   0x08

typedef int (*fs_mount_t)(struct vnode *parent, struct cdev *dev, struct vnode **root);

struct fs_ops {
    fs_mount_t          mount;
};

struct filesystem {
    char *              name;
    struct fs_ops *     ops;
};

struct mount {
    struct cdev *     device;
    struct filesystem * filesystem;
    uint64_t            flags;
};

int fs_mount(struct vnode *root, struct cdev *dev, const char *fsname, const char *path, int flags);
int fs_open(struct cdev *dev, struct vnode **root, const char *fsname, int flags);
void fs_register(char *name, struct fs_ops *ops);

#endif
