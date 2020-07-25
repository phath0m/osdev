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
#ifndef _SYS_FILE_H
#define _SYS_FILE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <sys/cdev.h>
#include <sys/dirent.h>
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
struct file *vfs_duplicate_file(struct file *);

int         fop_close(struct file *);
int         fop_fchmod(struct file *, mode_t);
int         fop_fchown(struct file *, uid_t, gid_t);
int         fop_ftruncate(struct file *, off_t);
int         fop_close(struct file *);
int         fop_creat(struct proc *, struct file **, const char *, mode_t);
int         fop_getdev(struct file *, struct cdev **);
int         fop_getvn(struct file *, struct vnode **);
int         fop_ioctl(struct file *, uint64_t, void *);
int         fop_mmap(struct file *, uintptr_t, size_t, int, off_t);
int         fop_read(struct file *, char *, size_t);
int         fop_readdirent(struct file *, struct dirent *);
int         fop_seek(struct file *, off_t, int);
int         fop_stat(struct file *, struct stat *);
int         fop_write(struct file *, const char *, size_t);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_FILE_H */
