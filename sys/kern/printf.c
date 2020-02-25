#include <stdarg.h>
#include <sys/device.h>
#include <sys/string.h>
#include <sys/systm.h>

/* actual output device*/
static struct device *output_device = NULL;

/* sets the kernel output to the specified device */
void
set_kernel_output(struct device *dev)
{
    output_device = dev;
}

void
puts(const char *str)
{   
    size_t nbyte = strlen(str);

    if (output_device) {
        device_write(output_device, str, nbyte, 0);
    }
}   

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

    puts(buf);
}

static inline void
vprint_p(uintptr_t arg)
{
    char buf[16];

    itoa_u(arg, buf, 16);

    int len = strlen(buf);
    int padding = (sizeof(uintptr_t) * 2) - len;

    for (int i = 0; i < padding; i++) {
        puts("0");
    }

    puts(buf);
}

static inline void
vprint_s(const char *arg)
{
    puts(arg);
}

static inline void
vprint_x(unsigned int arg)
{
    char buf[16];
    
    itoa_u(arg, buf, 16);

    puts(buf);
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
                    puts("%");
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
            puts(buf);
        }
    }
}
