#include <stdarg.h>
#include <stdio.h>
#include <sys/kernel.h>

void
panic(const char *fmt, ...)
{
    va_list arg;

    printf("panic: ");

    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);

    shutdown();
}
