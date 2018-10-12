/*
 * Basic driver for interacting with textscreen
 */

#include <rtl/types.h>
#include <sys/device.h>
#include <sys/i686/portio.h>
// remove me
#include <rtl/printf.h>

#define PORT0   0x3F8
#define PORT1   0x2F8
#define PORT2   0x3E8
#define PORT3   0x2E8

struct serial_state {
    int     port;
};

static int serial_close(struct device *dev);
static int serial_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
static int serial_open(struct device *dev);
static int serial_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);
static int serial_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

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

struct device serial0_device = {
    .name   =   "ttyS0",
    .close  =   serial_close,
    .ioctl  =   serial_ioctl,
    .open   =   serial_open,
    .read   =   serial_read,
    .write  =   serial_write,
    .state  =   &serial0_state
};

struct device serial1_device = {
    .name   =   "ttyS1",
    .close  =   serial_close,
    .ioctl  =   serial_ioctl,
    .open   =   serial_open,
    .read   =   serial_read,
    .write  =   serial_write,
    .state  =   &serial1_state
};

struct device serial2_device = {
    .name   =   "ttyS2",
    .close  =   serial_close,
    .ioctl  =   serial_ioctl,
    .open   =   serial_open,
    .read   =   serial_read,
    .write  =   serial_write,
    .state  =   &serial2_state
};

struct device serial3_device = {
    .name   =   "ttyS3",
    .close  =   serial_close,
    .ioctl  =   serial_ioctl,
    .open   =   serial_open,
    .read   =   serial_read,
    .write  =   serial_write,
    .state  =   &serial3_state
};

static void
echo_char(struct serial_state *state, char ch)
{
    int port = state->port;

    while ((io_read_byte(port + 5) & 0x20) == 0);

    io_write_byte(port, ch);
}

static char
read_char(struct serial_state *state)
{
    int port = state->port;

    while ((io_read_byte(port + 5) & 0x1) == 0);

    char ch = io_read_byte(port);
    
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
serial_close(struct device *dev)
{
    return 0;
}

static int
serial_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
{
    return 0;
}

static int
serial_open(struct device *dev)
{
    return 0;
}

static int
serial_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
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
serial_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct serial_state *state = (struct serial_state*)dev->state;

    for (int i = 0; i < nbyte; i++) {
        echo_char(state, buf[i]);
    }

    return nbyte;
}

__attribute__((constructor)) static void
serial_register()
{
    io_write_byte(PORT0 + 1, 0x00); // disable all interupts
    io_write_byte(PORT0 + 3, 0x80);  // enable DLAB
    io_write_byte(PORT0 + 0, 0x01);
    io_write_byte(PORT0 + 1, 0x00);
    io_write_byte(PORT0 + 3, 0x03);
    io_write_byte(PORT0 + 2, 0xC7);
    io_write_byte(PORT0 + 4, 0x0B);
    device_register(&serial0_device);
}

