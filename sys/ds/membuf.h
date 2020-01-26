#ifndef _DS_MEMBUF_H
#define _DS_MEMBUF_H

#include <ds/list.h>
#include <sys/mutex.h>
#include <sys/types.h>

#define MEMBUF_BLOCK_SIZE   2048

#define MEMBUF_ACTUAL_SIZE(p)   (LIST_SIZE(&p->blocks)*MEMBUF_BLOCK_SIZE)
#define MEMBUF_SIZE(p)          (p->size)

struct membuf {
    spinlock_t  lock;
    size_t      size;    
    struct list blocks;
};

void membuf_clear(struct membuf *mb);
void membuf_destroy(struct membuf *mb);
void membuf_expand(struct membuf *mb, size_t newsize);

struct membuf *membuf_new();
void membuf_write(struct membuf *mb, const void *buf, size_t nbyte, off_t pos);
size_t membuf_read(struct membuf *mb, void *buf, size_t nbyte, off_t pos);

#endif
