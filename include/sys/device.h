#ifndef SYS_DEVICE_H
#define SYS_DEVICE_H

#include <sys/types.h>

struct device;

typedef int (*dev_close_t)(struct device *dev);
typedef int (*dev_ioctl_t)(struct device *dev, uint64_t request, uintptr_t argp);
typedef int (*dev_open_t)(struct device *dev);
typedef int (*dev_read_t)(struct device *dev, char *buf, size_t nbyte, uint64_t pos);
typedef int (*dev_write_t)(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

struct device {
    char *      name;
    int         mode;
    dev_close_t close;
    dev_ioctl_t ioctl;
    dev_open_t  open;
    dev_read_t  read;
    dev_write_t write;
    void *      state;
};

int device_close(struct device *dev);
int device_destroy(struct device *dev);
int device_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
int device_open(struct device *dev);
int device_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);
int device_register(struct device *dev);
int device_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

#endif
