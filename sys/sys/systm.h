#ifndef SYS_KERNEL_H
#define SYS_KERNEL_H

#include <stdarg.h>
#include <sys/vnode.h>
#include <sys/device.h>

#define ELYSIUM_SYSNAME     "Elysium"
#define ELYSIUM_RELEASE     "v0.0.1"
#define ELYSIUM_VERSION     __DATE__ " "__TIME__ 

#define KASSERT(expr, msg) \
    if (!(expr)) { \
        panic("kassert: %s", msg); \
    }

int kmain(const char *args);
void kputs(const char *str);
void kset_output(struct device *dev);
void panic(const char *fmt, ...);
void shutdown();
void create_pipe(struct file **files);
void stacktrace(int max_frames);
void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list arg);

#endif
