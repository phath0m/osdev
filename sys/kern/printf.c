/*
 * printf.c - Textual output from kernel space
 *
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
#include <sys/cdev.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>

/* kernel ringbuf */
struct ringbuf {
    int         head;
    int         tail;
    int         size;
    int         written;
    char        buf[];
};


/* actual output device*/
static struct cdev *output_device = NULL;

/* kernel ringbuf */
static struct ringbuf *kmsg_rb = NULL;

static void
ringbuf_write(struct ringbuf *rb, const char *buf, size_t nbytes)
{
    int j;

    for (j = 0; j < nbytes; j++) {
        rb->buf[rb->tail] = buf[j];

        if (rb->tail == rb->head && rb->written != 0) {
            rb->head = (rb->head + 1) % rb->size;
        }

        rb->tail = (rb->tail + 1) % rb->size;

    }

    rb->written += nbytes;
}

static int
ringbuf_read(struct ringbuf *rb, int off, char *buf, size_t nbytes)
{
    int i;
    int j;

    for (i = (rb->head + off) % rb->size, j = 0; (i % rb->size) != rb->tail-1 && j < nbytes; i++, j++) {
        buf[j] = rb->buf[i % rb->size];
    }

    return j;
}


/* sets the kernel output to the specified device */
void
set_kernel_output(struct cdev *dev)
{
    output_device = dev;
}

int
get_kmsgs(char *buf, int off, size_t nbyte)
{
    return ringbuf_read(kmsg_rb, off, buf, nbyte);
}

int
get_kmsgs_size()
{
    if (kmsg_rb->size < kmsg_rb->written) {
        return kmsg_rb->size;
    }

    return kmsg_rb->written;
}

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

void
puts(const char *str)
{   
    size_t nbyte;

    nbyte = strlen(str);

    if (!kmsg_rb) {
        kmsg_rb = calloc(1, sizeof(*kmsg_rb) + 65535);
        kmsg_rb->size = 65535;
    }

    ringbuf_write(kmsg_rb, str, nbyte);

    if (output_device) {
        CDEVOPS_WRITE(output_device, str, nbyte, 0);
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
    int i;
    int len;
    int padding;
    char buf[16];

    itoa_u(arg, buf, 16);

    len = strlen(buf);
    padding = (sizeof(uintptr_t) * 2) - len;

    for (i = 0; i < padding; i++) {
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
    char ch;
    char spec;
    int i;
    int fmt_length;

    char buf[2];

    fmt_length = strlen(fmt);

    for (i = 0; i < fmt_length; i++) {
        ch = fmt[i];

        if (ch == '%') {
            spec = fmt[++i];

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
            buf[0] = fmt[i];
            buf[1] = 0;
            puts(buf);
        }
    }
}
