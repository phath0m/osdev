/*
 * Basic driver for interacting with textscreen
 */

#include <sys/device.h>
#include <sys/types.h>
#include <sys/i686/portio.h>
// remove me
#include <stdio.h>


static int keyboard_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);

struct device keyboard_device = {
    .name   =   "kbd",
    .mode   =   0600,
    .close  =   NULL,
    .ioctl  =   NULL,
    .isatty =   NULL,
    .open   =   NULL,
    .read   =   keyboard_read,
    .write  =   NULL,
    .state  =   NULL
};


static uint8_t
read_scancode()
{
    while ((io_read_byte(0x64) & 1) == 0);

    return io_read_byte(0x60);
}

static int
keyboard_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    for (int i = 0; i < nbyte; i++) {
        buf[i] = read_scancode();
    }

    return nbyte;
}

__attribute__((constructor))
static void
_init_keyboard()
{
    device_register(&keyboard_device);
}

