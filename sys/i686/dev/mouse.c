/*
 * mouse.c - PS/2 mouse driver
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
#include <machine/portio.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/interrupt.h>
#include <sys/types.h>

static int mouse_attach(struct driver *driver, struct device *dev);
static int mouse_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);

struct driver mouse_driver = {
    .attach     =   mouse_attach,
    .deattach   =   NULL,
    .probe      =   NULL
};

static uint8_t mouse_x;
static uint8_t mouse_y;
static uint8_t mouse_buttons;
static bool mouse_packet_ready;

//static int received_mouse_data = 0;

static void
mouse_wait(int a_type)
{
    int timeout;
    
    timeout = 100000;

    if (a_type) {
        while (timeout-- && (io_read8(0x64) & 0x2) == 0);
    } else {
        while (timeout-- && (io_read8(0x64) & 0x1) == 0);
    }
}

static void
mouse_send_cmd(uint8_t cmd)
{
    mouse_wait(1);
    io_write8(0x64, 0xD4);
    mouse_wait(1);
    io_write8(0x60, cmd);
}

static uint8_t
mouse_recv_resp()
{
    mouse_wait(0);
    return io_read8(0x60);
}

static int
mouse_irq_handler(struct device *dev, int inum)
{
    static int mouse_cycle = 0;
    static uint8_t mouse_data[4];

    uint8_t data;

    data = io_read8(0x60);

    switch (mouse_cycle) {
        case 0x00: {
            if (mouse_packet_ready) break;
            if ((data & 0x8) == 0) break; /* out of sync */
            mouse_data[mouse_cycle++] = data;
            break;
        }
        case 0x01:
            mouse_data[mouse_cycle++] = data;
            break;
        default:
            mouse_data[mouse_cycle] = data;
            mouse_buttons = mouse_data[0];
            mouse_x = mouse_data[1];
            mouse_y = mouse_data[2];
            mouse_cycle = 0;
            mouse_packet_ready = true;
            //__sync_lock_test_and_set(&received_mouse_data, 1);
            break;
        
    }
    return 0;
}

static int
mouse_attach(struct driver *driver, struct device *dev)
{
    int status;

    struct cdev_ops mouse_ops;
    struct cdev *cdev;


    /* Enable mouse */
    io_write8(0x64, 0xA8);
    mouse_wait(1);

    io_write8(0x64, 0x20);
    mouse_wait(0);

    status = io_read8(0x60) | 0x02;

    mouse_wait(1);

    io_write8(0x64, 0x60);
    mouse_wait(1);

    io_write8(0x60, status);

    mouse_send_cmd(0xF6);
    mouse_recv_resp();

    mouse_send_cmd(0xF4);
    mouse_recv_resp();

    irq_register(dev, 12, mouse_irq_handler);

    mouse_ops = (struct cdev_ops) {
        .close  = NULL,
        .init   = NULL,
        .ioctl  = NULL,
        .isatty = NULL,
        .mmap   = NULL,
        .open   = NULL,
        .read   = mouse_read,
        .write  = NULL
    };

    cdev = cdev_new("mouse", 0666, DEV_MAJOR_MOUSE, 0, &mouse_ops, NULL);

    irq_register(dev, 12, mouse_irq_handler);

    if (cdev && cdev_register(cdev) == 0) {
        return 0;
    }

    return -1;
}

static int
mouse_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    if (nbyte != 3) {
        return 0;
    }

//    while (!received_mouse_data) {
//        __sync_lock_test_and_set(&received_mouse_data, 0);
//    }

    buf[0] = mouse_buttons;
    buf[1] = mouse_x;
    buf[2] = mouse_y;

    mouse_packet_ready = false;

    return nbyte;
}
