/*
 * dev_init.c - Machine dependent device initialization
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

extern struct cdev kbd_device;
extern struct cdev lfb_device;
extern struct cdev mouse_device;
extern struct cdev serial0_device;
extern struct cdev serial1_device;
extern struct cdev serial2_device;
extern struct cdev serial3_device;
extern struct cdev vga_device;

struct cdev *machine_dev_all[] = {
#ifdef ENABLE_DEV_KBD
    &kbd_device,
#endif

#ifdef ENABLE_DEV_LFB
    &lfb_device,
#endif

#ifdef ENABLE_DEV_MOUSE
    &mouse_device,
#endif

#ifdef ENABLE_DEV_SERIAL
    &serial0_device,
    &serial1_device,
    &serial2_device,
    &serial3_device,
#endif

#ifdef ENABLE_DEV_VGA
    &vga_device,
#endif
    NULL
};

void
machine_dev_init()
{
    int i = 0;
    struct cdev *dev;

    while ((dev = machine_dev_all[i++])) {
        cdev_register(dev);
    }
}
