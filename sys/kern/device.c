/*
 * device.c
 *
 * Character device implementation
 *
 * Copyright (C) 2020
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
#include <ds/list.h>
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/types.h>

struct list device_list;

struct cdev *
cdev_from_devno(dev_t devno)
{
    uint16_t dev_minor = minor(devno);
    uint16_t dev_major = major(devno);

    list_iter_t iter;

    list_get_iter(&device_list, &iter);

    struct cdev *dev;
    struct cdev *ret = NULL;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (dev->majorno == dev_major && dev->minorno == dev_minor) {
            ret = dev;
            break;
        }
    }


    iter_close(&iter);
    
    return ret;
}

int
cdev_close(struct cdev *dev)
{
    if (dev->close) {
        return dev->close(dev);
    }

    return -(ENOTSUP);
}

int
cdev_destroy(struct cdev *dev)
{
    return 0;
}

int
cdev_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    if (dev->ioctl) {
        return dev->ioctl(dev, request, argp);
    }

    return -(ENOTSUP);
}

int
cdev_isatty(struct cdev *dev)
{
    if (dev->isatty) {
        return dev->isatty(dev);
    }

    return 0;
}

int
cdev_mmap(struct cdev *dev, uintptr_t addr, size_t size, int prot, off_t offset)
{
    if (dev->mmap) {
        return dev->mmap(dev, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

int
cdev_open(struct cdev *dev)
{
    if (dev->open) {
        return dev->open(dev);
    }

    return -(ENOTSUP);
}

int
cdev_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    if (dev->read) {
        return dev->read(dev, buf, nbyte, pos);
    }

    return -(ENOTSUP);
}

int
cdev_register(struct cdev *dev)
{
    if (dev->init) {
        dev->init(dev);
    }

    list_append(&device_list, dev);

    return 0;
}

int
cdev_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    if (dev->write) {
        return dev->write(dev, buf, nbyte, pos);
    }

    return -(ENOTSUP);
}
