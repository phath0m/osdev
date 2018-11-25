#ifndef SYS_KERNEL_H
#define SYS_KERNEL_H

#include <sys/vfs.h>
#include <sys/device.h>

int kmain();
void kputs(const char *str);
void kset_output(struct device *dev);
void panic(const char *fmt, ...);
void shutdown();
void create_pipe(struct file **files);

#endif
