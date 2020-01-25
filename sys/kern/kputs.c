#include <sys/device.h>
#include <sys/string.h>

static struct device *output_device;

void
kputs(const char *str)
{   
    size_t nbyte = strlen(str);

    if (output_device) {
        device_write(output_device, str, nbyte, 0);
    }
}   

void
kset_output(struct device *dev)
{
    output_device = dev;
}
