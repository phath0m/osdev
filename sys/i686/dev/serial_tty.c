/*
 * serial_tty.c - Serial device driver
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
#include <sys/types.h>

#define PORT0   0x3F8
#define PORT1   0x2F8
#define PORT2   0x3E8
#define PORT3   0x2E8

struct serial_state {
    int     port;
};

static int serial_attach(struct driver *driver, struct device *dev);
static int serial_close(struct cdev *dev);
static int serial_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
static int serial_isatty(struct cdev *dev);
static int serial_open(struct cdev *dev);
static int serial_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
static int serial_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

struct serial_state serial0_state = {
    .port   =   PORT0
};

struct serial_state serial1_state = {
    .port   =   PORT1
};

struct serial_state serial2_state = {
    .port   =   PORT2
};

struct serial_state serial3_state = {
    .port   =   PORT3
};

struct cdev serial0_device = {
    .name       =   "ttyS0",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   0,
    .close      =   serial_close,
    .init       =   NULL,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial0_state
};

struct cdev serial1_device = {
    .name       =   "ttyS1",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   1,
    .close      =   serial_close,
    .init       =   NULL,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial1_state
};

struct cdev serial2_device = {
    .name       =   "ttyS2",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   2,
    .close      =   serial_close,
    .init       =   NULL,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial2_state
};

struct cdev serial3_device = {
    .name       =   "ttyS3",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   3,
    .close      =   serial_close,
    .init       =   NULL,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial3_state
};

struct driver serial_driver = {
    .attach     =   serial_attach,
    .deattach   =   NULL,
    .probe      =   NULL
};

static void
echo_char(struct serial_state *state, char ch)
{
    int port = state->port;

    while ((io_read8(port + 5) & 0x20) == 0);

    io_write8(port, ch);
}

static char
read_char(struct serial_state *state)
{
    int port = state->port;

    while ((io_read8(port + 5) & 0x1) == 0);

    char ch = io_read8(port);
    
    switch (ch) {
        case 0x0D:
            return '\n';
        case 0x7F:
            return 0x08;
        default:
            return ch;
    }
}

static int
serial_close(struct cdev *dev)
{
    return 0;
}

static void
serial_init_device(struct cdev *cdev)
{
    struct serial_state *state = (struct serial_state*)cdev->state;

    int port = state->port;

    io_write8(port + 1, 0x00); // disable all interupts
    io_write8(port + 3, 0x80);  // enable DLAB
    io_write8(port + 0, 0x01);
    io_write8(port + 1, 0x00);
    io_write8(port + 3, 0x03);
    io_write8(port + 2, 0xC7);
    io_write8(port + 4, 0x0B);
    
    cdev_register(cdev);
}

static int
serial_attach(struct driver *driver, struct device *dev)
{
    serial_init_device(&serial0_device);
    serial_init_device(&serial1_device);
    serial_init_device(&serial2_device);
    serial_init_device(&serial3_device);

    return 0;
}

static int
serial_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    return 0;
}

static int
serial_isatty(struct cdev *dev)
{
    return 1;
}

static int
serial_open(struct cdev *dev)
{
    return 0;
}

static int
serial_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    struct serial_state *state = (struct serial_state*)dev->state;

    for (int i = 0; i < nbyte; i++) {
        char ch = read_char(state);

        echo_char(state, ch);

        buf[i] = ch;
        
        if (ch == '\n') {
            return i + 1;
        }
    }

    return nbyte;
}

static int
serial_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct serial_state *state = (struct serial_state*)dev->state;

    for (int i = 0; i < nbyte; i++) {
        echo_char(state, buf[i]);
    }

    return nbyte;
}
