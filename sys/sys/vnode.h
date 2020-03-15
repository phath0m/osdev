/*
 * vfs.h
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
#ifndef _SYS_VFS_H
#define _SYS_VFS_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <ds/dict.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/limits.h>
#include <sys/pool.h>
#include <sys/proc.h>

#define SEEK_SET    0x00
#define SEEK_CUR    0x01
#define SEEK_END    0x02

#define VN_INC_REF(p) __sync_fetch_and_add(&(p)->refs, 1)
#define VN_DEC_REF(p) if (__sync_fetch_and_sub(&(p)->refs, 1) == 1) vn_destroy(p);
#define INC_FILE_REF(p) __sync_fetch_and_add(&(p)->refs, 1)

struct dirent;
struct file;
struct fs_ops;
struct stat;
struct vnode;

typedef int (*vn_chmod_t)(struct vnode *node, mode_t mode);
typedef int (*vn_chown_t)(struct vnode *node, uid_t owner, uid_t group);
typedef int (*vn_node_destroy_t)(struct vnode *node);
typedef int (*vn_close_t)(struct vnode *node, struct file *fp);
typedef int (*vn_creat_t)(struct vnode *node, struct vnode **result, const char *name, mode_t mode);
typedef int (*vn_duplicate_t)(struct vnode *node, struct file *fp);
typedef int (*vn_ioctl_t)(struct vnode *node, uint64_t request, void *arg);
typedef int (*vn_lookup_t)(struct vnode *parent, struct vnode **result, const char *name);
typedef int (*vn_mkdir_t)(struct vnode *parent, const char *name, mode_t mode); 
typedef int (*vn_mknod_t)(struct vnode *parent, const char *name, mode_t mode, dev_t dev);
typedef int (*vn_mmap_t)(struct vnode *node, uintptr_t addr, size_t size, int prot, off_t offset);
typedef int (*vn_readdirent_t)(struct vnode *node, struct dirent *dirent, uint64_t entry);
typedef int (*vn_read_t)(struct vnode *node, void *buf, size_t nbyte, uint64_t pos);
typedef int (*vn_rmdir_t)(struct vnode *node, const char *path);
typedef int (*vn_seek_t)(struct vnode *node, off_t *pos, off_t off, int whence);
typedef int (*vn_stat_t)(struct vnode *node, struct stat *stat);
typedef int (*vn_truncate_t)(struct vnode *node, off_t length);
typedef int (*vn_unlink_t)(struct vnode *parent, const char *name);
typedef int (*vn_utimes_t)(struct vnode *node, struct timeval times[2]);
typedef int (*vn_write_t)(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos);

/* vnode methods */
struct vops {
    vn_node_destroy_t        destroy;
    vn_chmod_t               chmod;
    vn_chown_t               chown;
    vn_close_t               close;
    vn_creat_t               creat;
    vn_duplicate_t           duplicate;
    vn_ioctl_t               ioctl;
    vn_lookup_t              lookup;
    vn_read_t                read;
    vn_readdirent_t          readdirent;
    vn_rmdir_t               rmdir;
    vn_mkdir_t               mkdir;
    vn_mknod_t               mknod;
    vn_mmap_t                mmap;
    vn_seek_t                seek;
    vn_stat_t                stat;
    vn_truncate_t            truncate;
    vn_unlink_t              unlink;
    vn_utimes_t              utimes;
    vn_write_t               write;
};

/*
 * vnode struct, a vnode represents a file within the actual virtual file
 * system. The contents are populated by the actual filesystem driver. All
 * traditional file operations (mkdir(), write(), unlink(), ect) are centered
 * around vnodes.
 */
struct vnode {
    struct cdev *       device;
    struct dict         children;
    struct vops *       ops;
    struct vnode *      mount;
    struct vnode *      parent;
    union {
        struct cdev *   device;
        struct list     fifo_readers;
        struct list     un_connections;
    } un;
    int                 mount_flags;
    bool                ismount;
    ino_t               inode;
    gid_t               gid;
    uid_t               uid;
    mode_t              mode;
    dev_t               devno;
    uint64_t            size;
    void *              state;
    int                 refs;
};


extern struct pool  vn_pool;

void            vn_destroy(struct vnode *node);
int             vn_lookup(struct vnode *parent, struct vnode **result, const char *name);
struct vnode *  vn_new(struct vnode *parent, struct cdev *dev, struct vops *ops);
int             vn_open(struct vnode *root, struct vnode *cwd, struct vnode **result, const char *path);


/*
 * new functions
 */
int             vop_fchmod(struct vnode *node, mode_t mode);
int             vop_fchown(struct vnode *node, uid_t owner, gid_t group);
int             vop_ftruncate(struct vnode *node, off_t length);
int             vop_read(struct vnode *node, char *buf, size_t nbyte, off_t offset);
int             vop_readdirent(struct vnode *node, struct dirent *dirent, int dirno);
int             vop_stat(struct vnode *node, struct stat *stat);
int             vop_write(struct vnode *node, const char *buf, size_t nbyte, off_t offset);

int             vfs_open(struct proc *proc, struct file **result, const char *path, int flags);
int             vfs_open_r(struct proc *proc, struct file **result, const char *path, int flags);
int             vfs_access(struct proc *proc, const char *path, int mode);
int             vfs_chmod(struct proc *proc, const char *path, mode_t mode);
int             vfs_chown(struct proc *proc, const char *path, uid_t owner, gid_t group);
int             vfs_creat(struct proc *proc, struct file **result, const char *path, mode_t mode);
int             vfs_mkdir(struct proc *proc, const char *path, mode_t mode);
int             vfs_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev);
int             vfs_rmdir(struct proc *proc, const char *path);
int             vfs_truncate(struct proc *proc, const char *path, off_t length);
int             vfs_unlink(struct proc *proc, const char *path);
int             vfs_utimes(struct proc *proc, const char *path, struct timeval times[2]);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_VFS_H */
