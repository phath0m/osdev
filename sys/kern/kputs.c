#include <rtl/string.h>
#include <sys/device.h>

static struct device *output_device;

void
kputs(const char *str)
{   
    size_t nbyte = strlen(str);

    if (output_device) {
        device_write(output_device, 0, str, nbyte);
    }
}   

void
kset_output(struct device *dev)
{
    output_device = dev;
}
