#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list arg);

#endif
