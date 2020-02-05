#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/types.h>

struct list device_list;

int
device_close(struct device *dev)
{
    if (dev->close) {
        return dev->close(dev);
    }

    return -(ENOTSUP);
}

int
device_destroy(struct device *dev)
{
    return 0;
}

int
device_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
{
    if (dev->ioctl) {
        return dev->ioctl(dev, request, argp);
    }

    return -(ENOTSUP);
}

int
device_isatty(struct device *dev)
{
    if (dev->isatty) {
        return dev->isatty(dev);
    }

    return 0;
}

int
device_mmap(struct device *dev, uintptr_t addr, size_t size, int prot, off_t offset)
{
    if (dev->mmap) {
        return dev->mmap(dev, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

int
device_open(struct device *dev)
{
    if (dev->open) {
        return dev->open(dev);
    }

    return -(ENOTSUP);
}

int
device_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    if (dev->read) {
        return dev->read(dev, buf, nbyte, pos);
    }

    return -(ENOTSUP);
}

int
device_register(struct device *dev)
{
    list_append(&device_list, dev);
    return 0;
}

int
device_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    if (dev->write) {
        return dev->write(dev, buf, nbyte, pos);
    }

    return -(ENOTSUP);
}
