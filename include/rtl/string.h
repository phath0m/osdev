#ifndef RTL_STRING_H
#define RTL_STRING_H

#include <stdarg.h>
#include <rtl/types.h>

int atoi(const char *str, int base);

/* not actually part of the ansii string.h */
char *itoa(int value, char *str, int base);
char *itoa_u(unsigned int value, char *str, int base);

void *memcpy(void *dest, const void *src, size_t nbyte);
void *memset(void *ptr, int value, size_t nbyte);

void sprintf(char *str, const char *fmt, ...);

char *strcpy(char *dest, const char *src);
int strlen(const char *str);
int strcmp(const char *str1, const char *str2);
char *strncpy(char *dest, const char *src, size_t nbyte);
int strncmp(const char *str1, const char *str2, size_t nbyte);
char *strrchr(const char *str, char ch);
void strreverse(char *str);
void vsprintf(char *str, const char *fmt, va_list arg);

#endif
