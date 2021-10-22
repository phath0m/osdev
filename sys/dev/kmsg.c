/*
 * kmsg.c - Kernel message buffer userspace interface
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
#include <sys/cdev.h>
#include <sys/devno.h>
#include <sys/errno.h>
#include <sys/systm.h>

static int kmsg_close(struct cdev *);
static int kmsg_open(struct cdev *);
static int kmsg_read(struct cdev *, char *, size_t, uint64_t);
static int kmsg_write(struct cdev *, const char *, size_t, uint64_t);

struct cdev kmsg_device = {
    .name       =   "kmsg",
    .mode       =   0666,
    .majorno    =   DEV_MAJOR_KMSG,
    .minorno    =   3,
    .ops.close      =   kmsg_close,
    .ops.ioctl      =   NULL,
    .ops.open       =   kmsg_open,
    .ops.read       =   kmsg_read,
    .ops.write      =   kmsg_write,
    .state      =   NULL
};

static int
kmsg_close(struct cdev *dev)
{
    return 0;
}

static int
kmsg_open(struct cdev *dev)
{
    return 0;
}

static int
kmsg_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    return get_kmsgs(buf, pos, nbyte);
}

static int
kmsg_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    return nbyte;
}

void
kmsg_device_init()
{
    cdev_register(&kmsg_device);
}
