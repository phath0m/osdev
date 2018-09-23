#ifndef RTL_STRING_H
#define RTL_STRING_H

#include <stdarg.h>
#include <rtl/types.h>

/* not actually part of the ansii string.h */
char *itoa(int value, char *str, int base);
char *itoa_u(unsigned int value, char *str, int base);

void sprintf(char *str, const char *fmt, ...);

char *strcpy(char *dest, const char *src);
int strlen(const char *str);
char *strncpy(char *dest, const char *src, size_t nbyte);
void strreverse(char *str);
void vsprintf(char *str, const char *fmt, va_list arg);

#endif
