/*
 * file.h
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
#ifndef _ELYSIUM_SYS_FILE_H
#define _ELYSIUM_SYS_FILE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <sys/cdev.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vnode.h>

struct file;

typedef int (*f_chmod_t)(struct file *, mode_t);
typedef int (*f_chown_t)(struct file *, uid_t, uid_t);
typedef int (*f_close_t)(struct file *);
typedef int (*f_destroy_t)(struct file *);
typedef int (*f_duplicate_t)(struct file *);
typedef int (*f_getdev_t)(struct file *, struct cdev **);
typedef int (*f_getvn_t)(struct file *, struct vnode **);
typedef int (*f_ioctl_t)(struct file *, uint64_t, void *);
typedef int (*f_mmap_t)(struct file *, uintptr_t, size_t, int, off_t);
typedef int (*f_readdirent_t)(struct file *, struct dirent *, uint64_t);
typedef int (*f_read_t)(struct file *, void *, size_t);
typedef int (*f_seek_t)(struct file *, off_t *, off_t, int);
typedef int (*f_stat_t)(struct file *, struct stat *);
typedef int (*f_truncate_t)(struct file *, off_t);
typedef int (*f_write_t)(struct file *, const void *, size_t);


/* file operations */
struct fops {
    f_chmod_t       chmod;
    f_chown_t       chown;
    f_close_t       close;
    f_destroy_t     destroy;
    f_duplicate_t   duplicate;
    f_getdev_t      getdev;
    f_getvn_t       getvn;
    f_ioctl_t       ioctl;
    f_mmap_t        mmap;
    f_readdirent_t  readdirent;
    f_read_t        read;
    f_seek_t        seek;
    f_stat_t        stat;
    f_truncate_t    truncate;
    f_write_t       write;
};

/* 
 * represents an open file. Usually, this corresponds to a file descriptor in
 * user space. This can be thought of being the equivelent of C's FILE struct
 */
struct file {
    struct fops *       ops;
    int                 flags;
    int                 refs;
    void *              state;
    off_t               position;
};

extern struct pool  file_pool;

#define FILE_POSITION(fp) ((fp)->position)

struct file *file_new(struct fops *, void *);
struct file *file_duplicate(struct file *);

int         file_close(struct file *);
int         fop_creat(struct proc *, struct file **, const char *, mode_t);

__attribute__((always_inline))
static inline int
FOP_FCHMOD(struct file *fp, mode_t mode)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->chmod) {
        return ops->chmod(fp, mode);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_FCHOWN(struct file *fp, uid_t owner, gid_t group)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->chown) {
        return ops->chown(fp, owner, group);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_FTRUNCATE(struct file *fp, off_t length)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->truncate) {
        return ops->truncate(fp, length);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_GETDEV(struct file *fp, struct cdev **result)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->getdev) {
        return ops->getdev(fp, result);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_GETVN(struct file *fp, struct vnode **result)
{
    struct fops *ops;
    
    ops = fp->ops;
    
    if (ops && ops->getvn) {
        return ops->getvn(fp, result);
    }
    
    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_IOCTL(struct file *fp, uint64_t request, void *arg)
{
    struct fops *ops = fp->ops;

    if (ops->ioctl) {
        return ops->ioctl(fp, request, arg);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_MMAP(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->mmap) {
        return ops->mmap(fp, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline int
FOP_READ(struct file *fp, char *buf, size_t nbyte)
{
    int read;
    struct fops *ops;

    if (fp->flags == O_WRONLY) {
        return -(EPERM);
    }

    ops = fp->ops;

    if (ops->read) {
        read = ops->read(fp, buf, nbyte);
    
        if (read > 0) {
            fp->position += read;
        }

        return read;
    }

    return -(EPERM);
}

__attribute__((always_inline))
static inline int
FOP_READDIRENT(struct file *fp, struct dirent *dirent)
{
    int res;
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->readdirent) {
        res = ops->readdirent(fp, dirent, fp->position);

        if (res == 0) {
            fp->position++;
        }

        return res;
    }

    return -(EPERM);
}

__attribute__((always_inline))
static inline int
FOP_SEEK(struct file *fp, off_t off, int whence)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->seek) {
        return ops->seek(fp, &fp->position, off, whence);
    }

    return -(ESPIPE);
}

__attribute__((always_inline))
static inline int
FOP_STAT(struct file *fp, struct stat *stat)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->stat) {
        return ops->stat(fp, stat);
    }

    return -(ENOTSUP);
}

__attribute__((always_inline))
static inline off_t
FOP_TELL(struct file *fp)
{
    return fp->position;
}

__attribute__((always_inline))
static inline int
FOP_WRITE(struct file *fp, const char *buf, size_t nbyte)
{
    ssize_t written;
    struct fops *ops;

    if (fp->flags == O_RDONLY) {
        return -(EPERM);
    }

    ops = fp->ops;

    if (ops->write) {
        written = ops->write(fp, buf, nbyte);

        if (written > 0) {
            fp->position += (off_t)written;
        }

        return written;
    }

    return -(EPERM);
}

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_FILE_H */
