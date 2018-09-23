#include <stdarg.h>
#include <rtl/printf.h>
#include <rtl/string.h>

extern void kputs(const char *str);

void
printf(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);

    vprintf(fmt, arg);

    va_end(arg);
}

static inline void
vprint_d(int arg)
{
    char buf[16];

    itoa(arg, buf, 10);

    kputs(buf);
}

static inline void
vprint_s(const char *arg)
{
    kputs(arg);
}

static inline void
vprint_x(unsigned int arg)
{
    char buf[16];

    itoa_u(arg, buf, 16);

    kputs(buf);
}

static inline void
vprint_arg(const char spec, va_list arg)
{
    switch (spec) {
        case '%':
            kputs("%");
            break;
        case 'd':
            vprint_d(va_arg(arg, int));
            break;
        case 's':
            vprint_s(va_arg(arg, const char*));
            break;
        case 'x':
            vprint_x(va_arg(arg, unsigned int));
            break;
        default:
            break;
    }
}

void
vprintf(const char *fmt, va_list arg)
{
    int fmt_length = strlen(fmt);

    for (int i = 0; i < fmt_length; i++) {
        char ch = fmt[i];

        if (ch == '%') {
            vprint_arg(fmt[++i], arg);
        } else {
            char buf[2];
            buf[0] = fmt[i];
            buf[1] = 0;
            kputs(buf);
        }
    }
}
