/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdarg.h>
#include <sys/string.h>
#include <sys/types.h>

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)

int
atoi(const char *str, int base)
{
    int len = strlen(str);

    int result = 0;
    int place_value = 1;

    for (int i = len - 1; i >= 0; i--) {
        char c = str[i];

        int digit;

        if (c >= 'a' && c <='f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            digit = c - '0';
        }

        result = (digit * place_value) + result;

        place_value *= base;
    }

    return result;
}

char *
itoa(int value, char *str, int base)
{
    int rem = 0;
    int i = 0;

    do {
        rem = value % base;
        value = (value - rem) / base;
        str[i++] = (char)(rem + (rem < 10 ? '0' : 'A' - 10));
    } while (value > 0);
    
    str[i++] = 0;

    strreverse(str);

    return str;
}

char *
itoa_u(unsigned int value, char *str, int base)
{
    int rem = 0;
    int i = 0;

    do {
        rem = value % base;
        value = (value - rem) / base;
        str[i++] = (char)(rem + (rem < 10 ? '0' : 'A' - 10));
    } while (value > 0);

    str[i++] = 0;

    strreverse(str);

    return str;
}

int
memcmp(const void *buf1, const void *buf2, size_t nbyte)
{
    const char *str1 = buf1;
    const char *str2 = buf2;

    for (int i = 0; i < nbyte; i++) {
        char c1 = str1[i];
        char c2 = str2[i];

        if (c1 != c2) {
            return c1 - c2;
        }
    }

    return 0;
}

void *
memcpy(void *dest, const void *src, size_t nbyte)
{
    char *dest_buf = (char*)dest;
    char *src_buf = (char*)src;


    for (int i = 0; i < nbyte; i++) {
        *(dest_buf++) = *(src_buf++);
    }

    return dest;
}

void *
memset(void *ptr, int value, size_t nbyte)
{
    char *buf = (char*)ptr;

    for (int i = 0; i < nbyte; i++) {
        buf[i] = 0;
    }

    return ptr;
}

void
sprintf(char *str, const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);

    vsprintf(str, fmt, arg);

    va_end(arg);
}

char *
strchr(const char *str, char ch)
{
    int len = strlen(str);
    char *ret = NULL;

    for (int i = 0; i < len; i++) {
        if (str[i] == ch) {
            ret = (char*)(str + i);
        }
    }

    return ret;
}

char *
strcpy(char *dest, const char *src)
{
    size_t len = strlen(src);

    return strncpy(dest, src, len + 1);
}

int
strlen(const char *str)
{
    int len = 0;

    while (*str) {
        len++;
        str++;
    }

    return len;
}

int
strcmp(const char *str1, const char *str2)
{
    return strncmp(str1, str2, MIN(strlen(str1), strlen(str2)));
}

char *
strncpy(char *dest, const char *src, size_t nbyte)
{
    size_t len = strlen(src);
    size_t maxlen = nbyte - 1; // minus one for null terminator

    int i;
    for (i = 0; i < maxlen && i < len; i++) {
        dest[i] = src[i];
    }

    dest[i] = 0;

    return dest;
}

int
strncmp(const char *str1, const char *str2, size_t nbyte)
{
    int len1 = strlen(str1);
    int len2 = strlen(str2);

    int minlen = MIN(len1, len2);

    minlen = MIN(minlen, nbyte);

    for (int i = 0; i < minlen; i++) {
        char c1 = str1[i];
        char c2 = str2[i];

        if (c1 != c2) {
            return c1 - c2;
        }
    }

    if (len1 != len2) {
        return -1;
    }

    return 0;
}

char *
strrchr(const char *str, char ch)
{
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        if (str[i] == ch) {
            return (char*)(str + i);
        }
    }

    return NULL;
}

void
strreverse(char *str)
{
    int len = strlen(str);
    int mid = len / 2;

    for (int i = 1; i <= mid; i++) {
        char c1 = str[i - 1];
        char c2 = str[len - i];
        str[i - 1] = c2;
        str[len - i] = c1;
    }
}

static inline int
vsprint_c(char *str, int ch)
{
    *str = (char)ch;

    return 1;
}

static inline int
vsprint_d(char *str, int arg)
{
    char buf[16];

    itoa(arg, buf, 10);
    strcpy(str, buf);

    return strlen(buf);
}

static inline int
vsprint_s(char *str, const char *arg)
{
    int len = strlen(arg);

    strcpy(str, arg);

    return len;
}

static inline int
vsprint_x(char *str, unsigned int arg)
{
    char buf[16];

    itoa_u(arg, buf, 16);
    strcpy(str, buf);

    return strlen(buf);
}

static inline int
vsprint_arg(char *str, const char spec, va_list arg)
{
    switch (spec) {
        case '%':
            *str = '%';
            return 1;
        case 'c':
            return vsprint_c(str, va_arg(arg, int));
        case 'd':
            return vsprint_d(str, va_arg(arg, int));
        case 's':
            return vsprint_s(str, va_arg(arg, const char*));
        case 'x':
            return vsprint_x(str, va_arg(arg, unsigned int));
        default:
            return 0;
    }
}

void
vsprintf(char *str, const char *fmt, va_list arg)
{
    int fmt_length = strlen(fmt);

    for (int i = 0; i < fmt_length; i++) {
        char ch = fmt[i];

        if (ch == '%') {
            str += vsprint_arg(str, fmt[++i], arg);
        } else {
            *(str++) = fmt[i];
        }
    }
    *str = 0;
}

