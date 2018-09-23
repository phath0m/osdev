#ifndef SYS_KERNEL_H
#define SYS_KERNEL_H

#include <sys/device.h>

int kmain();
void kputs(const char *str);
void kset_output(struct device *dev);

#endif
