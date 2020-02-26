#include <sys/device.h>
#include <sys/devices.h>
#include <sys/errno.h>

static int full_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);
static int null_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
static int pseudo_close(struct cdev *dev);
static int pseudo_open(struct cdev *dev);
static int zero_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
static int zero_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

struct cdev full_device = {
    .name       =   "full",
    .mode       =   0666,
    .majorno    =   DEV_MAJOR_PSEUDO,
    .minorno    =   0,
    .close      =   pseudo_close,
    .ioctl      =   NULL,
    .open       =   pseudo_open,
    .read       =   zero_read,
    .write      =   full_write
};

struct cdev null_device = {
    .name       =   "null",
    .mode       =   0666,
    .majorno    =   DEV_MAJOR_PSEUDO,
    .minorno    =   1,
    .close      =   pseudo_close,
    .ioctl      =   NULL,
    .open       =   pseudo_open,
    .read       =   null_read,
    .write      =   zero_write
};

struct cdev zero_device = {
    .name       =   "zero",
    .mode       =   0666,
    .majorno    =   DEV_MAJOR_PSEUDO,
    .minorno    =   2,
    .close      =   pseudo_close,
    .ioctl      =   NULL,
    .open       =   pseudo_open,
    .read       =   zero_read,
    .write      =   zero_write,
    .state      =   NULL
};

static int
full_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    return -(ENOSPC);
}

static int
null_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    return -1;
}

static int
pseudo_close(struct cdev *dev)
{
    return 0;
}

static int
pseudo_open(struct cdev *dev)
{
    return 0;
}

static int
zero_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    for (int i = 0; i < nbyte; i++) {
        buf[i] = 0;
    }

    return nbyte;
}

static int
zero_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    return nbyte;
}

__attribute__((constructor))
void
_init_pseudo_devices()
{
    cdev_register(&full_device);
    cdev_register(&null_device);
    cdev_register(&zero_device);
}
