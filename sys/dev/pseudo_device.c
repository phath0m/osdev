/*
 * pseudo_device.c - Generic devices (null,full,zero)
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
#include <sys/device.h>
#include <sys/devno.h>
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

void
pseudo_devices_init()
{
    cdev_register(&full_device);
    cdev_register(&null_device);
    cdev_register(&zero_device);
}
