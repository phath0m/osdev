#include <ds/fifo.h>
#include <sys/device.h>
#include <sys/devices.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/i686/portio.h>

static int keyboard_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);

struct device keyboard_device = {
    .name       =   "kbd",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_KBD,
    .minorno    =   0,
    .close      =   NULL,
    .ioctl      =   NULL,
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
keyboard_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    while (FIFO_EMPTY(keyboard_buf)) {
        thread_yield();
    }

    return fifo_read(keyboard_buf, buf, nbyte);
}

__attribute__((constructor))
static void
_init_keyboard()
{
    keyboard_buf = fifo_new(4096);
    device_register(&keyboard_device);
    register_intr_handler(33, keyboard_irq_handler);
}

