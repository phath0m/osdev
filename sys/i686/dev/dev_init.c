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
#include <machine/pci.h>
#include <sys/device.h>
#include <sys/cdev.h>
#include <sys/systm.h>

extern struct driver kbd_driver;
extern struct driver lfb_driver;
extern struct driver mouse_driver;
extern struct driver rtc_driver;
extern struct driver serial_driver;
extern struct driver vga_driver;
extern struct driver virtio_blk_driver;

struct driver *machine_driver_all[] = {
#ifdef ENABLE_DEV_SERIAL
    &serial_driver,
#endif

#ifdef ENABLE_DEV_VIRTIO
    &virtio_blk_driver,
#endif

#ifdef ENABLE_DEV_KBD
    &kbd_driver,
#endif

#ifdef ENABLE_DEV_LFB
    &lfb_driver,
#endif

#ifdef ENABLE_DEV_MOUSE
    &mouse_driver,
#endif

#ifdef ENABLE_DEV_SERIAL
#endif

#ifdef ENABLE_DEV_VGA
    &vga_driver,
#endif
    &rtc_driver,
    NULL
};

void
machine_dev_init()
{
    int i;
    struct driver *driver;

    pci_init();

    i = 0;

    while ((driver = machine_driver_all[i++])) {
        driver_register(driver);
    }
#ifdef ENABLE_DEV_VGA
    set_kernel_output(&vga_device);
#elif ENABLE_DEV_LFB
    //set_kernel_output(&lfb_device);
#endif
}
