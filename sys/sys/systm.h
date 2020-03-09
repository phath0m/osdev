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

int     kmain(const char *args);
void    puts(const char *str);
void    set_kernel_output(struct cdev *dev);
void    panic(const char *fmt, ...);
void    shutdown();
void    stacktrace(int max_frames);
void    printf(const char *fmt, ...);
void    vprintf(const char *fmt, va_list arg);

void    ksym_declare(const char *name, uintptr_t val);
int     ksym_find_nearest(uintptr_t needle, uintptr_t *dist, char *buf, size_t bufsize);
int     ksym_resolve(const char *name, uintptr_t *val);

#endif
