#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
vprint_p(uintptr_t arg)
{
    char buf[16];

    itoa_u(arg, buf, 16);

    int len = strlen(buf);
    int padding = (sizeof(uintptr_t) * 2) - len;

    for (int i = 0; i < padding; i++) {
        kputs("0");
    }

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

void
vprintf(const char *fmt, va_list arg)
{
    int fmt_length = strlen(fmt);

    for (int i = 0; i < fmt_length; i++) {
        char ch = fmt[i];

        if (ch == '%') {
            char spec = fmt[++i];

            switch (spec) {
                case '%':
                    kputs("%");
                    break;
                case 'd':
                    vprint_d(va_arg(arg, int));
                    break;
                case 'p':
                    vprint_p(va_arg(arg, uintptr_t));
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

        } else {
            char buf[2];
            buf[0] = fmt[i];
            buf[1] = 0;
            kputs(buf);
        }
    }
}
