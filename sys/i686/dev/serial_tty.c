#include <machine/portio.h>
#include <sys/device.h>
#include <sys/devices.h>
#include <sys/types.h>

#define PORT0   0x3F8
#define PORT1   0x2F8
#define PORT2   0x3E8
#define PORT3   0x2E8

struct serial_state {
    int     port;
};

static int serial_close(struct device *dev);
static int serial_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
static int serial_isatty(struct device *dev);
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
    .name       =   "ttyS0",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   0,
    .close      =   serial_close,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial0_state
};

struct device serial1_device = {
    .name       =   "ttyS1",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   1,
    .close      =   serial_close,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial1_state
};

struct device serial2_device = {
    .name       =   "ttyS2",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   2,
    .close      =   serial_close,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial2_state
};

struct device serial3_device = {
    .name       =   "ttyS3",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_TTYS,
    .minorno    =   3,
    .close      =   serial_close,
    .ioctl      =   serial_ioctl,
    .isatty     =   serial_isatty,
    .open       =   serial_open,
    .read       =   serial_read,
    .write      =   serial_write,
    .state      =   &serial3_state
};

static void
echo_char(struct serial_state *state, char ch)
{
    int port = state->port;

    while ((io_read_byte(port + 5) & 0x20) == 0);

    io_write_byte(port, ch);
}

static void
init_serial_device(int port)
{
    io_write_byte(port + 1, 0x00); // disable all interupts
    io_write_byte(port + 3, 0x80);  // enable DLAB
    io_write_byte(port + 0, 0x01);
    io_write_byte(port + 1, 0x00);
    io_write_byte(port + 3, 0x03);
    io_write_byte(port + 2, 0xC7);
    io_write_byte(port + 4, 0x0B);
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
serial_isatty(struct device *dev)
{
    return 1;
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

__attribute__((constructor))
void
_init_serial_tty()
{
    init_serial_device(PORT0);
    init_serial_device(PORT1);
    init_serial_device(PORT2);
    init_serial_device(PORT3);

    device_register(&serial0_device);
    device_register(&serial1_device);
    device_register(&serial2_device);
    device_register(&serial3_device);
}

