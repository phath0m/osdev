#include <stdarg.h>
#include <rtl/string.h>
#include <rtl/types.h>

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

void
sprintf(char *str, const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);

    vsprintf(str, fmt, arg);

    va_end(arg);
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
