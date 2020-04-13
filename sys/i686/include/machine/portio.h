/*
 * portio.h
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
#ifndef _SYS_PORTIO_H
#define _SYS_PORTIO_H

#include <sys/types.h>

static inline uint8_t
io_read_byte(uint16_t port)
{
    uint8_t res;

    asm volatile("inb %1, %0" : "=a"(res) : "Nd"(port));

    return res;
}

static inline uint16_t
io_read_short(uint16_t port)
{
    uint16_t res;

    asm volatile("inw %1, %0" : "=a"(res) : "Nd"(port));

    return res;
}

static inline uint32_t
io_read_long(uint16_t port)
{
    uint32_t res;

    asm volatile("inl %1, %0" : "=a"(res) : "Nd"(port));

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

static inline void
io_write_long(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

#endif
