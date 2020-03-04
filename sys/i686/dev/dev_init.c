#include <sys/device.h>

extern struct cdev kbd_device;
extern struct cdev lfb_device;
extern struct cdev mouse_device;
extern struct cdev serial0_device;
extern struct cdev serial1_device;
extern struct cdev serial2_device;
extern struct cdev serial3_device;
extern struct cdev vga_device;

struct cdev *machine_dev_all[] = {
#ifdef ENABLE_DEV_KBD
    &kbd_device,
#endif

#ifdef ENABLE_DEV_LFB
    &lfb_device,
#endif

#ifdef ENABLE_DEV_MOUSE
    &mouse_device,
#endif

#ifdef ENABLE_DEV_SERIAL
    &serial0_device,
    &serial1_device,
    &serial2_device,
    &serial3_device,
#endif

#ifdef ENABLE_DEV_VGA
    &vga_device,
#endif
    NULL
};

void
machine_dev_init()
{
    int i = 0;
    struct cdev *dev;

    while ((dev = machine_dev_all[i++])) {
        cdev_register(dev);
    }
}
