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
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/types.h>

static int mouse_init(struct cdev *dev);
static int mouse_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);

struct cdev mouse_device = {
    .name       =   "mouse",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_MOUSE,
    .minorno    =   0,
    .close      =   NULL,
    .init       =   mouse_init,
    .ioctl      =   NULL,
    .isatty     =   NULL,
    .open       =   NULL,
    .read       =   mouse_read,
    .write      =   NULL,
    .state      =   NULL
};

static uint8_t mouse_x;
static uint8_t mouse_y;
static uint8_t mouse_buttons;
//static int received_mouse_data = 0;

static void
mouse_wait(int a_type)
{
    int timeout = 10000;

    if (a_type) {
        while (timeout-- && io_read_byte(0x64));
    } else {
        while (timeout-- && !(io_read_byte(0x64) & 1));
    }
}

static void
mouse_send_cmd(uint8_t cmd)
{
    mouse_wait(1);
    io_write_byte(0x64, 0xD4);
    mouse_wait(1);
    io_write_byte(0x60, cmd);
}

static uint8_t
mouse_recv_resp()
{
    mouse_wait(0);
    return io_read_byte(0x60);
}

static int
mouse_irq_handler(int inum, struct regs *regs)
{
    static int mouse_cycle;
    static uint8_t mouse_data[3];

    switch (mouse_cycle) {
        case 0x00:
        case 0x01:
            mouse_data[mouse_cycle++] = io_read_byte(0x60);
            break;
        default:
            mouse_data[mouse_cycle] = io_read_byte(0x60);
            mouse_buttons = mouse_data[0];
            mouse_x = mouse_data[1];
            mouse_y = mouse_data[2];
            mouse_cycle = 0;
            //__sync_lock_test_and_set(&received_mouse_data, 1);
            break;
        
    }
    return 0;
}

static int
mouse_init(struct cdev *dev)
{
    mouse_wait(1);
    io_write_byte(0x64, 0xA8);
    mouse_wait(1);
    io_write_byte(0x64, 0x20);
    mouse_wait(0);
    int status = io_read_byte(0x60) | 2;
    mouse_wait(1);
    io_write_byte(0x64, 0x60);
    mouse_wait(1);
    io_write_byte(0x60, status);

    mouse_send_cmd(0xF6);
    mouse_recv_resp();

    mouse_send_cmd(0xF4);
    mouse_recv_resp();

    bus_register_intr(44, mouse_irq_handler);

    return 0;
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

    return nbyte;
}
