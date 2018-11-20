#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list arg);

#endif
