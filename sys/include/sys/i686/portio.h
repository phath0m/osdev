#ifndef SYS_PORTIO_H
#define SYS_PORTIO_H

#include <sys/types.h>

static inline uint8_t
io_read_byte(uint16_t port)
{
    uint8_t res;

    asm volatile("inb %1, %0" : "=a"(res) : "Nd"(port));

    return res;
}

static inline void
io_write_byte(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void
io_write_short(uint16_t port, uint16_t val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

#endif
