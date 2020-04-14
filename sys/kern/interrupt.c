/*
 * interrupt.c - Implements functionality for registering interrupt handlers
 *
 * Note: This file will probably be axed in future refactoring efforts
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
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/types.h>

struct list device_all;
struct list driver_all;

int
device_register(struct device *dev)
{
    list_append(&device_all, dev);

   	list_iter_t iter;
    list_get_iter(&driver_all, &iter);

    struct driver *driver;

    while (iter_move_next(&iter, (void**)&driver)) {
        if (driver->attached) {
            continue;
        }

        if (driver->probe && driver->probe(driver, dev) == 0) {
			if (driver->attach(driver, dev) == 0) {
                driver->attached = true;
            }
        }
    }
 
    iter_close(&iter);

    return 0;
}

int
device_setup_intr(struct device *dev, int inum, dev_intr_t handler)
{
    return 0;
}

int
driver_register(struct driver *driver)
{
    if (!driver->attach) {
        return -1;
    }

    list_append(&driver_all, driver);

    struct device *dev;

    if (!driver->probe) {
        /* 
         * if we can't probe devices, we will make one for the driver 
         * 
         * This is for stuff that can't be enumerated and whose existence is assumed
         */
        dev = calloc(1, sizeof(struct device));
        
        if (driver->attach(driver, dev) == 0) {
            driver->attached = true;
            list_append(&device_all, dev);
        } else {
            free(dev);
        }

        return 0;
    }

    list_iter_t iter;
    list_get_iter(&device_all, &iter);

    while (iter_move_next(&iter, (void**)&dev)) {
        if (driver->probe(driver, dev) == 0) {
            driver->attach(driver, dev);
        }
    }

    iter_close(&iter);

    return 0;
}
