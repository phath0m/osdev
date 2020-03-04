#ifndef _SYS_DEVICE_H
#define _SYS_DEVICE_H

#include <sys/types.h>

#define minor(n) (n & 0xFF)
#define major(n) ((n >> 8) & 0xFF)
#define makedev(maj, min) (min | (maj << 8))

struct cdev;

/* character device methods */
typedef int (*cdev_close_t)(struct cdev *dev);
typedef int (*cdev_init_t)(struct cdev *dev);
typedef int (*cdev_ioctl_t)(struct cdev *dev, uint64_t request, uintptr_t argp);
typedef int (*cdev_isatty_t)(struct cdev *dev);
typedef int (*cdev_mmap_t)(struct cdev *dev, uintptr_t addr, size_t size, int prot, off_t offset);
typedef int (*cdev_open_t)(struct cdev *dev);
typedef int (*cdev_read_t)(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
typedef int (*cdev_write_t)(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

/* represents a character device */
struct cdev {
    char *          name;       /* name of the device */
    int             mode;       /* default mode to use for devfs */
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

struct vnode;

struct cdev *   cdev_from_devno(dev_t dev);
struct file *   cdev_to_file(struct vnode *host, dev_t devno);

int             cdev_close(struct cdev *dev);
int             cdev_destroy(struct cdev *dev);
int             cdev_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
int             cdev_isatty(struct cdev *dev);
int             cdev_mmap(struct cdev *dev, uintptr_t addr, size_t size, int prot, off_t offset);
int             cdev_open(struct cdev *dev);
int             cdev_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
int             cdev_register(struct cdev *dev);
int             cdev_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

#endif
