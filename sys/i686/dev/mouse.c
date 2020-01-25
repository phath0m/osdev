#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/types.h>
#include <sys/i686/portio.h>

static int mouse_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);

struct device mouse_device = {
    .name   =   "mouse",
    .mode   =   0600,
    .close  =   NULL,
    .ioctl  =   NULL,
    .isatty =   NULL,
    .open   =   NULL,
    .read   =   mouse_read,
    .write  =   NULL,
    .state  =   NULL
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
mouse_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
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

__attribute__((constructor))
static void
_init_mouse()
{
    device_register(&mouse_device);
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

    register_intr_handler(12, mouse_irq_handler);
    register_intr_handler(12+32, mouse_irq_handler);
}

