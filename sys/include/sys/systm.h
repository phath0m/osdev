#ifndef SYS_KERNEL_H
#define SYS_KERNEL_H

#include <sys/vfs.h>
#include <sys/device.h>

#define ELYSIUM_SYSNAME     "Elysium"
#define ELYSIUM_RELEASE     "v0.0.1"
#define ELYSIUM_VERSION     __DATE__ " "__TIME__ 

#define KASSERT(expr, msg) \
    if (!(expr)) { \
        panic("kassert: %s", msg); \
    }

int kmain();
void kputs(const char *str);
void kset_output(struct device *dev);
void panic(const char *fmt, ...);
void shutdown();
void create_pipe(struct file **files);

#endif
