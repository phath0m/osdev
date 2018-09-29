#include <rtl/list.h>
#include <rtl/types.h>
#include <sys/device.h>

struct list device_list;

int
device_close(struct device *dev)
{
    return dev->close(dev);
}

int
device_destroy(struct device *dev)
{
    return 0;
}

int
device_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
{
    return dev->ioctl(dev, request, argp);
}

int
device_open(struct device *dev)
{
    return dev->open(dev);
}

int
device_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    return dev->read(dev, buf, nbyte, pos);
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
    return dev->write(dev, buf, nbyte, pos);
}
