/*
 * virtio.h
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
#ifndef _DEV_VIRTIO_H
#define _DEV_VIRTIO_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__
#include <sys/types.h>

#define VIRTIO_ACKNOWLEDGE      1
#define VIRTIO_DRIVER           2
#define VIRTIO_FAILED           128
#define VIRTIO_FEATURES_OK      8
#define VIRTIO_DRIVER_OK        4

#define VIRTIO_BLK_F_SIZE_MAX   (1<<1)
#define VIRTIO_BLK_F_SEG_MAX    (1<<2)
#define VIRTIO_BLK_F_GEOMETRY   (1<<4)
#define VIRTIO_BLK_F_RO         (1<<5)
#define VIRTIO_BLK_F_BLK_SIZE   (1<<6)
#define VIRTIO_BLK_F_TOPOLOGY   (1<<10)
#define VIRTIO_BLK_F_MQ         (1<<12)
#define VIRTQ_DESC_F_NEXT       1 
#define VIRTQ_DESC_F_WRITE      2 
#define VIRTQ_DESC_F_INDIRECT   4 

struct virtq_desc {
    uint64_t    address;
    uint32_t    length;
    uint16_t    flags;
    uint16_t    next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t    flags;
    uint16_t    index;
    uint16_t    rings[];
} __attribute__((packed));

struct virtq_used_elem {
    uint32_t    index;
    uint32_t    length;
} __attribute__((packed));

struct virtq_used {
    uint16_t                flags;
    uint16_t                index;
    struct virtq_used_elem	rings[];
} __attribute__((packed));

struct virtq {
    struct virtq_desc   *   buffers;
    struct virtq_avail  *   available;
    struct virtq_used   *   used;
    int                     buffer_idx;
    int                     size;
};

struct virtio_dev {
    uint32_t        iobase;
    struct virtq    queues[16];
};

struct virtq_buffer {
    void *  buf;
    int     length;
    int     flags;
};

int         virtio_attach(struct device *dev);
int         virtq_send(struct device *dev, int queue_idx, struct virtq_buffer *buffers, int nbuffers);
int         virtq_recv(struct device *dev);
uint8_t     virtio_read8(struct device *, int);
uint16_t    virtio_read16(struct device *, int);
uint32_t    virtio_read32(struct device *, int);

#endif /* __KERNEL__*/
#ifdef __cplusplus
}
#endif
#endif /* _DEV_VIRTIO_H */
