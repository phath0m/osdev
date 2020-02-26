#include <ds/fifo.h>
#include <sys/malloc.h>
#include <sys/types.h>


struct fifo *
fifo_new(size_t maxsize)
{
    struct fifo *fifo = calloc(1, sizeof(struct fifo));
    fifo->buf = calloc(1, maxsize);
    fifo->buf_size = maxsize;
    return fifo;
}

size_t
fifo_read(struct fifo *fifo, void *buf, size_t nbyte)
{
    uint8_t *buf8 = (uint8_t*)buf;

    spinlock_lock(&fifo->lock);

    if (nbyte > fifo->size) {
        nbyte = fifo->size;
    }

    for (int i = 0; i < nbyte; i++) {
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
    uint8_t *buf8 = (uint8_t*)buf;
    
    spinlock_lock(&fifo->lock);

    for (int i = 0; i < nbyte; i++) {
        fifo->buf[(i + fifo->tail_pos) % fifo->buf_size] = buf8[i];
    }

    fifo->tail_pos += nbyte;
    fifo->size += nbyte;

    spinlock_unlock(&fifo->lock);
  
    return 0;
}
