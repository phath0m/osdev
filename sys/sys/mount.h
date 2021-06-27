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

#include <sys/types.h>
#include <sys/file.h>
#include <sys/vnode.h>

typedef int     (*fs_mount_t)(struct vnode *, struct file *, struct vnode **);
typedef bool    (*fs_probe_t)(struct cdev *, int, const uint8_t *);

struct fs_ops {
    fs_mount_t          mount;
    fs_probe_t          probe;
};

struct filesystem {
    char *              name;
    struct fs_ops *     ops;
};

struct mount {
    struct cdev         *   device;
    struct filesystem   *   filesystem;
    uint64_t                flags;
};

int     fs_mount(struct vnode *, const char *, const char *, const char *, int);
int     fs_open(struct file *, struct vnode **, const char *, int);
bool    fs_probe(struct cdev *, const char *, int, const uint8_t *);
void    fs_register(char *, struct fs_ops *);

void    devfs_init();
void    tmpfs_init();
void    ext2_init();

#endif /* __KERNEL__ */

int     mount(const char *, const char *, const char *, unsigned long, const void *);

#ifdef __cplusplus
}
#endif
#endif /* _SYS_MOUNT_H */
