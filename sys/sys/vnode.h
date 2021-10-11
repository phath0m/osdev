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
#ifndef _ELYSIUM_SYS_VNODE_H
#define _ELYSIUM_SYS_VNODE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <ds/dict.h>
#include <sys/cdev.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
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

typedef int (*vn_chmod_t)(struct vnode *, mode_t);
typedef int (*vn_chown_t)(struct vnode *, uid_t, uid_t);
typedef int (*vn_node_destroy_t)(struct vnode *);
typedef int (*vn_close_t)(struct vnode *, struct file *);
typedef int (*vn_creat_t)(struct vnode *, struct vnode **, const char *, mode_t);
typedef int (*vn_duplicate_t)(struct vnode *, struct file *);
typedef int (*vn_ioctl_t)(struct vnode *, uint64_t, void *);
typedef int (*vn_lookup_t)(struct vnode *, struct vnode **, const char *);
typedef int (*vn_mkdir_t)(struct vnode *, const char *, mode_t); 
typedef int (*vn_mknod_t)(struct vnode *, const char *, mode_t, dev_t);
typedef int (*vn_mmap_t)(struct vnode *, uintptr_t, size_t size, int, off_t);
typedef int (*vn_readdirent_t)(struct vnode *, struct dirent *, uint64_t);
typedef int (*vn_read_t)(struct vnode *, void *, size_t, uint64_t);
typedef int (*vn_rmdir_t)(struct vnode *, const char *);
typedef int (*vn_seek_t)(struct vnode *, off_t *, off_t, int);
typedef int (*vn_stat_t)(struct vnode *, struct stat *);
typedef int (*vn_truncate_t)(struct vnode *, off_t);
typedef int (*vn_unlink_t)(struct vnode *, const char *);
typedef int (*vn_utimes_t)(struct vnode *, struct timeval[2]);
typedef int (*vn_write_t)(struct vnode *, const void *, size_t, uint64_t);

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

void            vn_destroy(struct vnode *);
int             vn_lookup(struct vnode *, struct vnode **, const char *);
struct vnode *  vn_new(struct vnode *, struct cdev *, struct vops *);
int             vn_open(struct vnode *, struct vnode *, struct vnode **, const char *);

void            vn_resolve_name(struct vnode *, char *, int);


int             vfs_open(struct proc *, struct file **, const char *, int);
int             vfs_open_r(struct proc *, struct file **, const char *, int);
int             vfs_access(struct proc *, const char *, int);
int             vfs_chmod(struct proc *, const char *, mode_t);
int             vfs_chown(struct proc *, const char *, uid_t, gid_t);
int             vfs_creat(struct proc *, struct file **, const char *, mode_t);
int             vfs_mkdir(struct proc *, const char *, mode_t);
int             vfs_mknod(struct proc *, const char *, mode_t, dev_t);
int             vfs_rmdir(struct proc *, const char *);
int             vfs_truncate(struct proc *, const char *, off_t);
int             vfs_unlink(struct proc *, const char *);
int             vfs_utimes(struct proc *, const char *, struct timeval[2]);


/* macros */
__attribute__((always_inline))
static inline int     
VOP_FCHMOD(struct vnode *vn, mode_t mode)
{       
    struct vops *ops;
    
    ops = vn->ops;
    
    if (ops && ops->chmod) {
        return ops->chmod(vn, mode);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
VOP_FCHOWN(struct vnode *vn, uid_t owner, gid_t group)
{
    struct vops *ops;
    
    ops = vn->ops;

    if (ops && ops->chown) {
        return ops->chown(vn, owner, group);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
VOP_FTRUNCATE(struct vnode *vn, off_t length)
{
    struct vops *ops;
    
    ops = vn->ops;

    if (ops && ops->truncate) {
        return ops->truncate(vn, length);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
VOP_READ(struct vnode *vn, char *buf, size_t nbyte, off_t offset)
{
    struct vops *ops;
    
    ops = vn->ops;

    if (ops && ops->read) {
        return ops->read(vn, buf, nbyte, offset);
    }

    return -(EPERM);
}

__attribute__((always_inline))
static inline int
VOP_READDIRENT(struct vnode *vn, struct dirent *dirent, int dirno)
{
    struct vops *ops;
    
    ops = vn->ops;

    if (ops && ops->readdirent) {
        return ops->readdirent(vn, dirent, dirno);
    }

    return -(EPERM);
}

__attribute__((always_inline))
static inline int
VOP_STAT(struct vnode *vn, struct stat *stat)
{
    struct vops *ops;
    
    ops = vn->ops;

    if (ops->stat) {
        return ops->stat(vn, stat);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
VOP_WRITE(struct vnode *vn, const char *buf, size_t nbyte, off_t offset)
{
    int written;
    struct vops *ops;
    
    ops = vn->ops;

    if (ops->write) {
        written = ops->write(vn, buf, nbyte, offset);

        return written;
    }

    return -(EPERM);
}
#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_VNODE_H */
