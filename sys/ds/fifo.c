/*
 * fifo.c - Generic FIFO buffer implementation
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
#include <ds/fifo.h>
#include <sys/malloc.h>
#include <sys/types.h>

struct fifo *
fifo_new(size_t maxsize)
{
    struct fifo *fifo;

    fifo = calloc(1, sizeof(struct fifo));
    fifo->buf = calloc(1, maxsize);
    fifo->buf_size = maxsize;

    return fifo;
}

size_t
fifo_read(struct fifo *fifo, void *buf, size_t nbyte)
{
    int i;
    uint8_t *buf8;

    buf8 = (uint8_t*)buf;

    spinlock_lock(&fifo->lock);

    if (nbyte > fifo->size) {
        nbyte = fifo->size;
    }

    for (i = 0; i < nbyte; i++) {
        buf8[i] = fifo->buf[i % fifo->buf_size + fifo->head_pos];
    }

    fifo->size -= nbyte;
    fifo->head_pos += nbyte;

    if (fifo->size == 0) {
        fifo->head_pos = 0;
        fifo->tail_pos = 0;
    }

    spinlock_unlock(&fifo->lock);

    return nbyte;
}

size_t
fifo_write(struct fifo *fifo, void *buf, size_t nbyte)
{
    int i;
    uint8_t *buf8;

    buf8 = (uint8_t*)buf;
    
    spinlock_lock(&fifo->lock);

    for (i = 0; i < nbyte; i++) {
        fifo->buf[(i + fifo->tail_pos) % fifo->buf_size] = buf8[i];
    }

    fifo->tail_pos += nbyte;
    fifo->size += nbyte;

    spinlock_unlock(&fifo->lock);
  
    return 0;
}
