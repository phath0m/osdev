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
#include <sys/types.h>

intr_handler_t intr_handlers[256];

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
driver_register(struct driver *driver)
{
    list_append(&driver_all, driver);

    list_iter_t iter;
    list_get_iter(&device_all, &iter);

    struct device *dev;

    while (iter_move_next(&iter, (void**)&dev)) {
        if (driver->probe && driver->probe(driver, dev) == 0) {
            driver->attach(driver, dev);
        }
    }

    iter_close(&iter);

    return 0;
}

void
dispatch_to_intr_handler(int inum, struct regs *regs)
{
    intr_handler_t handler = intr_handlers[inum];

    if (handler) {
        handler(inum, regs);
    }
}

int
intr_register(int inum, intr_handler_t handler)
{
    intr_handlers[inum] = handler;
    
    return 0;
}
