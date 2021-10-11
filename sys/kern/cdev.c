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
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/types.h>

struct list device_list;

struct cdev *
cdev_from_devno(dev_t devno)
{
    list_iter_t iter;
    uint16_t dev_minor;
    uint16_t dev_major;

    struct cdev *dev;
    struct cdev *ret;

    dev_minor = minor(devno);
    dev_major = major(devno);

    list_get_iter(&device_list, &iter);

    ret = NULL;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (dev->majorno == dev_major && dev->minorno == dev_minor) {
            ret = dev;
            break;
        }
    }


    iter_close(&iter);
    
    return ret;
}

struct cdev * 
cdev_new(const char *name, int mode, int majorno, int minorno, struct cdev_ops *ops,
    void *state)
{
    struct cdev *dev;
    
    dev = calloc(1, sizeof(struct cdev) + strlen(name) + 1);

    strcpy((char*)&dev[1], name);

    dev->name = (char*)&dev[1];
    dev->majorno = majorno;
    dev->minorno = minorno;
    dev->mode = mode;
    dev->state = state;

    memcpy(&dev->ops, ops, sizeof(*ops));

    return dev;
}

int
cdev_destroy(struct cdev *dev)
{
    return 0;
}

int
cdev_register(struct cdev *dev)
{
    if (dev->ops.init) {
        dev->ops.init(dev);
    }

    list_append(&device_list, dev);

    return 0;
}