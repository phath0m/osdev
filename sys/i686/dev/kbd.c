/*
 * kdb.c - PS/2 keyboard driver
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
#include <ds/fifo.h>
#include <machine/portio.h>
#include <sys/cdev.h>
#include <sys/devno.h>
#include <sys/errno.h>
#include <sys/interrupt.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/types.h>

static int keyboard_init(struct cdev *dev);
static int keyboard_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
static int keyboard_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);

struct cdev kbd_device = {
    .name       =   "kbd",
    .mode       =   0666,
    .majorno    =   DEV_MAJOR_KBD,
    .minorno    =   0,
    .close      =   NULL,
    .init       =   keyboard_init,
    .ioctl      =   keyboard_ioctl,
    .isatty     =   NULL,
    .open       =   NULL,
    .read       =   keyboard_read,
    .write      =   NULL,
    .state      =   NULL
};


static struct fifo *keyboard_buf;

static int
keyboard_irq_handler(int inum, struct regs *regs)
{
    while ((io_read_byte(0x64) & 2));

    uint8_t scancode = io_read_byte(0x60);
    fifo_write(keyboard_buf, &scancode, 1);

    return 0;
}

static int
keyboard_init(struct cdev *dev)
{
    keyboard_buf = fifo_new(4096);
    intr_register(33, keyboard_irq_handler);

    return 0;
}

static int      
keyboard_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{   
    switch (request) {
        case FIONREAD:
            *((uint32_t*)argp) = FIFO_SIZE(keyboard_buf);
            return -1;
    }

    return -1;
}

static int
keyboard_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    while (FIFO_EMPTY(keyboard_buf)) {
        if (thread_exit_requested()) {
            return -(EINTR);
        }

        thread_yield();
    }

    return fifo_read(keyboard_buf, buf, nbyte);
}
