/*
 * mount.h
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
#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H
#ifdef __cplusplus
extern "C" {
#endif

#define MS_RDONLY   0x01
#define MS_NOEXEC   0x08

#ifdef __KERNEL__
#include <sys/vnode.h>

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

int     fs_mount(struct vnode *root, struct cdev *dev, const char *fsname, const char *path, int flags);
int     fs_open(struct cdev *dev, struct vnode **root, const char *fsname, int flags);
void    fs_register(char *name, struct fs_ops *ops);
void    devfs_init();
void    tmpfs_init();

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_MOUNT_H */
