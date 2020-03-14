/*
 * membuf.c - Dynamic buffer implementation that can be resized
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
#include <ds/list.h>
#include <ds/membuf.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/systm.h>

void
membuf_expand(struct membuf *mb, size_t newsize)
{
    int requested_blocks = (newsize / MEMBUF_BLOCK_SIZE) + 1; 

    for (int i = 0; i < requested_blocks; i++) {
        void *newbuf = calloc(1, MEMBUF_BLOCK_SIZE);

        list_append(&mb->blocks, newbuf);
    }

    mb->size = newsize;
}

static void *
get_block_buf(struct membuf *mb, off_t pos)
{
    void *buf;
    int i = 0;
    void *ret = NULL;

    int block_index = pos / MEMBUF_BLOCK_SIZE;

    list_iter_t iter;

    list_get_iter(&mb->blocks, &iter);

    while (iter_move_next(&iter, &buf)) {
        if (i == block_index) {
            ret = buf;
            break;
        }
        i++;        
    }

    iter_close(&iter);

    return ret;
}

void
membuf_clear(struct membuf *mb)
{
    list_destroy(&mb->blocks, true);
    memset(&mb->blocks, 0, sizeof(struct list));
}

void
membuf_destroy(struct membuf *mb)
{
    list_destroy(&mb->blocks, true);
    free(mb);
}

struct membuf *
membuf_new()
{
    struct membuf *new_mb = calloc(1, sizeof(struct membuf));

    return new_mb;
}

size_t
membuf_read(struct membuf *mb, void *buf, size_t nbyte, off_t pos)
{
    spinlock_lock(&mb->lock);
    
    off_t cur_pos = pos;

    if (pos + nbyte > MEMBUF_SIZE(mb)) {
        nbyte = MEMBUF_SIZE(mb) - pos;
    }

    if (nbyte <= 0) {
        return 0;
    }

    int required_blocks = (nbyte / MEMBUF_BLOCK_SIZE) + 1;
    size_t bytes_read = 0;

    for (int i = 0; i < required_blocks; i++) {
        int relative_offset = cur_pos % MEMBUF_BLOCK_SIZE;
        int byte_count = nbyte - (i * MEMBUF_BLOCK_SIZE);

        if (byte_count > (MEMBUF_BLOCK_SIZE - relative_offset)) byte_count = MEMBUF_BLOCK_SIZE - relative_offset;

        void *block = get_block_buf(mb, cur_pos);

        memcpy(buf + (i * MEMBUF_BLOCK_SIZE), block + relative_offset, byte_count);
        bytes_read += byte_count;
        cur_pos += byte_count;
    }
      
    spinlock_unlock(&mb->lock);

    return bytes_read;
}

void
membuf_write(struct membuf *mb, const void *buf, size_t nbyte, off_t pos)
{
    spinlock_lock(&mb->lock);

    off_t cur_pos = pos;
    size_t requested_size = pos + nbyte;

    if (requested_size > MEMBUF_ACTUAL_SIZE(mb)) {
        membuf_expand(mb, requested_size);
    }
    
    int required_blocks = (nbyte / MEMBUF_BLOCK_SIZE) + 1;
    size_t written = 0;

    for (int i = 0; i < required_blocks; i++) {
        int relative_offset = cur_pos % MEMBUF_BLOCK_SIZE;
        int byte_count = nbyte - (i * MEMBUF_BLOCK_SIZE);
        if (byte_count > (MEMBUF_BLOCK_SIZE - relative_offset)) byte_count = MEMBUF_BLOCK_SIZE - relative_offset;

        void *block = get_block_buf(mb, cur_pos);

        KASSERT(block != NULL, "get_block_buf should not equal NULL");

        memcpy(block + relative_offset, buf + (i * MEMBUF_BLOCK_SIZE), byte_count);
        written += byte_count;
        cur_pos += byte_count;
    }

    if (mb->size < cur_pos) {
        mb->size = cur_pos;
    }

    spinlock_unlock(&mb->lock);
}
