/*
 * virtio.c - underlying base functionality to communicate with Virtio
 * devices
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
#include <machine/pci.h>
#include <machine/portio.h>
#include <machine/vm.h>
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include "virtio.h"

#define ALIGN(x) (((x) + 4096) & 0xFFFFF000) 

static inline unsigned virtq_size(unsigned int qsz) 
{ 
     return ALIGN(sizeof(struct virtq_desc)*qsz + sizeof(uint16_t)*(3 + qsz)) 
          + ALIGN(sizeof(uint16_t)*3 + sizeof(struct virtq_used_elem)*qsz); 
}

static int
virtq_init(struct virtio_dev *vdev, uint16_t addr, int nelems)
{
    size_t bufsize = sizeof(struct virtq_desc)*nelems;
    size_t avail_size = 4 + (sizeof(uint16_t)*nelems);
    size_t total_size = virtq_size(nelems);

    uint8_t *buf = sbrk_a(total_size, 4096);

    vdev->queues[addr].buffers = (struct virtq_desc*)buf;
    vdev->queues[addr].available = (struct virtq_avail*)&buf[bufsize];
    vdev->queues[addr].used = (struct virtq_used*)(&buf[(bufsize + avail_size + 0xFFF) &~0xFFF]);
    vdev->queues[addr].size = nelems;

    memset(buf, 0, total_size);

    io_write16(vdev->iobase+0x0E, addr);
    io_write32(vdev->iobase+0x08, PAGE_INDEX(KVATOP(buf)));

    vdev->queues[addr].available->flags = 0;
    vdev->queues[addr].used->flags = 0;
    return 0;
}


int
virtq_send(struct device *dev, int queue_idx, struct virtq_buffer *buffers, int nbuffers)
{
    struct virtio_dev *vdev = dev->state;
    struct virtq *queue = &vdev->queues[queue_idx];

    int start_idx = queue->buffer_idx % queue->size;
    int avail_idx = queue->available->index % queue->size;

    queue->available->rings[avail_idx] = start_idx;

    for (int i = 0; i < nbuffers; i++) {
        struct virtq_buffer *buffer = &buffers[i];
        int buffer_idx = queue->buffer_idx % queue->size;
        queue->buffers[buffer_idx].length = buffer->length;
        queue->buffers[buffer_idx].address = (uint64_t)KVATOP(buffer->buf);
        queue->buffers[buffer_idx].flags = buffer->flags;

        if ((buffer->flags & VIRTQ_DESC_F_NEXT)) {
            queue->buffers[buffer_idx].next = (queue->buffer_idx + 1) % queue->size;
        } else {
            queue->buffers[buffer_idx].next = 0;
        }
        queue->buffer_idx++;
    }

    queue->available->index++;

    io_write8(vdev->iobase + 0x10, 0);

    bool acknowledged = false;    
    while (!acknowledged) {
        for (int i = 0; i < queue->size; i++) {
            if (queue->used->rings[i].length != 0 && queue->used->rings[i].index == start_idx) {
                queue->used->rings[i].length = 0;
                acknowledged = true;
                break;
            }
        }
    }

    return 0;
}

static int
virtio_irq_handler(struct device *dev, int inum)
{
    struct virtio_dev *vdev = dev->state;

    io_read8(vdev->iobase+0x13);
    return 0;
}

int
virtio_attach(struct device *dev)
{
    struct virtio_dev *vdev = calloc(1, sizeof(struct virtio_dev));
    vdev->iobase = PCI_IO_BASE(pci_get_config32(dev, PCI_CONFIG_BAR0));

    io_write8(vdev->iobase+0x12, 0);
    io_write8(vdev->iobase+0x12, VIRTIO_ACKNOWLEDGE);
    io_write8(vdev->iobase+0x12, VIRTIO_DRIVER | VIRTIO_ACKNOWLEDGE);

    uint32_t features = io_read32(vdev->iobase);

    features &= ~VIRTIO_BLK_F_RO;
    features &= ~VIRTIO_BLK_F_BLK_SIZE;
    features &= ~VIRTIO_BLK_F_TOPOLOGY;

    io_write32(vdev->iobase+0x04, features);
    io_write8(vdev->iobase+0x12, VIRTIO_FEATURES_OK | VIRTIO_DRIVER | VIRTIO_ACKNOWLEDGE);

    if ((io_read8(vdev->iobase+0x12) & VIRTIO_FEATURES_OK) == 0) {
        printf("virtio: features not accepted\n\r");
        return -1;
    }

    dev->state = vdev;

    int size = 0;
    uint16_t addr = 0;

    do {
        io_write16(vdev->iobase+0x0E, addr);
        size = io_read16(vdev->iobase+0x0C);

        if (size > 0) {
            virtq_init(vdev, addr, size);
            break;
        }

        addr++;
    } while (size != 0);

    int irq = pci_get_config8(dev, PCI_CONFIG_IRQLINE);

    dev->state = vdev;
    irq_register(dev, irq, virtio_irq_handler);
    io_write8(vdev->iobase+0x12, VIRTIO_DRIVER_OK | VIRTIO_FEATURES_OK | VIRTIO_DRIVER | VIRTIO_ACKNOWLEDGE);

    return 0;
}
