/*
 * bus.h
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
#ifndef _SYS_DEVICE_H
#define _SYS_DEVICE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <ds/list.h>
#include <machine/device.h>

struct device;
struct driver;

/* device driver method prototypes */
typedef int (*driver_probe_t)(struct driver *driver, struct device *dev);
typedef int (*driver_attach_t)(struct driver *driver, struct device *dev);
typedef int (*driver_deattach_t)(struct driver *driver, struct device *dev);

/* device is created by machdep portion for each device attached to host  */
struct device {
    int     type;           /* identify device type, values defined by machdep portion*/
    void *  state;          /* private */
};

/* driver struct allows for kernel-space code to identify and attach to discovered devices */
struct driver {
    driver_probe_t      probe;
    driver_attach_t     attach;
    driver_deattach_t   deattach;
    bool                attached;
    struct list         devices;
};

int     device_register(struct device *device);
int     driver_register(struct driver *driver);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _SYS_DEVICE_H */
