/*
 * cdev.h
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
#ifndef _SYS_CDEV_H
#define _SYS_CDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define minor(n) (n & 0xFF)
#define major(n) ((n >> 8) & 0xFF)
#define makedev(maj, min) (min | (maj << 8))

#ifdef __KERNEL__
struct cdev;

/* character device methods */
typedef int (*cdev_close_t)(struct cdev *);
typedef int (*cdev_init_t)(struct cdev *);
typedef int (*cdev_ioctl_t)(struct cdev *, uint64_t, uintptr_t);
typedef int (*cdev_isatty_t)(struct cdev *);
typedef int (*cdev_mmap_t)(struct cdev *, uintptr_t, size_t, int, off_t);
typedef int (*cdev_open_t)(struct cdev *);
typedef int (*cdev_read_t)(struct cdev *, char *, size_t, uint64_t);
typedef int (*cdev_write_t)(struct cdev *, const char *, size_t, uint64_t);

/* represents a character device */
struct cdev {
    char *          name;       /* name of the device */
    int             mode;       /* default mode to use for devfs */
    int             uid;        /* owner */
    int             majorno;    /* device major; identifies type of device */
    int             minorno;    /* device minor; identifies instance of device */
    cdev_close_t    close;
    cdev_init_t     init;
    cdev_ioctl_t    ioctl;
    cdev_isatty_t   isatty;
    cdev_mmap_t     mmap;
    cdev_open_t     open;
    cdev_read_t     read; 
    cdev_write_t    write;
    void *          state;      /* private data */
};

struct cdev_ops {
    cdev_close_t    close;
    cdev_init_t     init;
    cdev_ioctl_t    ioctl;
    cdev_isatty_t   isatty;
    cdev_mmap_t     mmap;
    cdev_open_t     open;
    cdev_read_t     read;
    cdev_write_t    write;
};

struct vnode;

struct cdev *	cdev_new(const char *, int, int, int, struct cdev_ops *, void *);
struct cdev *   cdev_from_devno(dev_t);
struct file *   cdev_to_file(struct vnode *, dev_t);

int             cdev_close(struct cdev *);
int             cdev_destroy(struct cdev *);
int             cdev_ioctl(struct cdev *, uint64_t, uintptr_t);
int             cdev_isatty(struct cdev *);
int             cdev_mmap(struct cdev *, uintptr_t, size_t, int, off_t);
int             cdev_open(struct cdev *);
int             cdev_read(struct cdev *, char *, size_t, uint64_t);
int             cdev_register(struct cdev *);
int             cdev_write(struct cdev *, const char *, size_t, uint64_t);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif 
#endif /* _SYS_CDEV_H */
