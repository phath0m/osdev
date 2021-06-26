/*
 * virtio_blk.c - virtio block devices
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
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include "virtio.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define VIRTIO_BLK_T_IN           0 
#define VIRTIO_BLK_T_OUT          1 
#define VIRTIO_BLK_T_FLUSH        4 

static int vblk_read(struct cdev *, char *, size_t, uint64_t);
static int vblk_write(struct cdev *, const char *, size_t, uint64_t);
static int vblk_probe(struct driver *, struct device *);
static int vblk_attach(struct driver *, struct device *);
static int vblk_deattach(struct driver *, struct device *);

struct virtio_blk_req {
    uint32_t    type;
    uint32_t    reserved;
    uint64_t    sector;
} __attribute__((packed));

struct driver virtio_blk_driver = {
    .attach     =   vblk_attach,
    .deattach   =   vblk_deattach,
    .probe      =   vblk_probe,
};

static int
vblk_write_sector(struct device *dev, uint64_t sector, void *data)
{
    uint8_t status;
    struct virtio_blk_req req;
    struct virtq_buffer *buffers;

    volatile uint8_t *status_p;

    status = 0xFF;

    req = (struct virtio_blk_req){
        .type = VIRTIO_BLK_T_OUT,
        .sector = sector,
        .reserved = 0
    };

    buffers = (struct virtq_buffer[]){
        {
            .length = 16,
            .flags = VIRTQ_DESC_F_NEXT,
            .buf = &req
        },
        {
            .length = 512,
            .flags = VIRTQ_DESC_F_NEXT,
            .buf = data
        },
        {
            .length = 1,
            .flags = VIRTQ_DESC_F_WRITE,
            .buf = &status
        }
    };

    virtq_send(dev, 0, buffers, 3);

    status_p = &status;
    while (*status_p == 0xFF);

    if (status != 0) {
        printf("virtio_blk: write_sector() error: sector=%d status=%d\n\r", sector, status);
        return -1;
    }
    return 0;
}

static int
vblk_read_sector(struct device *dev, uint64_t sector, void *data)
{
    uint8_t status;

    struct virtio_blk_req req;
    struct virtq_buffer *buffers;

    volatile uint8_t *status_p;

    status = 0xFF;
    
    req = (struct virtio_blk_req){
        .type = VIRTIO_BLK_T_IN,
        .sector = sector,
        .reserved = 0
    };

    buffers = (struct virtq_buffer[]){
        {
            .length = 16,
            .flags = VIRTQ_DESC_F_NEXT,
            .buf = &req
        },
        {
            .length = 512,
            .flags = VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT,
            .buf = data
        },
        {
            .length = 1,
            .flags = VIRTQ_DESC_F_WRITE,
            .buf = &status
        }
    };

    virtq_send(dev, 0, buffers, 3);

    status_p = &status;

    while (*status_p == 0xFF);

    if (status != 0) {
        panic("virtio_blk: read_sector() error: sector=%x status=%d\n\r", sector, status);
        return -1;
    }
    return 0;
}

static int
vblk_ioctl(struct cdev *cdev, uint64_t request, uintptr_t argp)
{
    uint64_t total_size;
    struct device *dev;
    
    dev = cdev->state;
    total_size = (uint64_t)virtio_read32(dev, 0x14)*512;

    switch (request) {
        case BLKGETSIZE:
            *((uint64_t*)argp) = total_size; 
            break;
    }

    return 0;
}

static int
vblk_read(struct cdev *cdev, char *buf, size_t nbyte, uint64_t pos)
{
    static uint8_t block[512];

    uint32_t bytes_read;
    uint32_t bytes_this_block;
    uint32_t i;
    uint64_t start_sector;
    uint32_t start_offset;

    struct device *dev;
    
    dev = cdev->state;

    bytes_read = 0;
    start_sector = pos >> 9;
    start_offset = (uint32_t)(pos & 0x1FF);

    for (i = 0; bytes_read < nbyte; i++) {
        bytes_this_block = MIN(512-start_offset, nbyte - bytes_read);

        vblk_read_sector(dev, start_sector + i, block);
        memcpy(&buf[bytes_read], &block[start_offset], bytes_this_block);

        start_offset = 0;
        bytes_read += bytes_this_block;
    }

    return bytes_read;
}

static int
vblk_write(struct cdev *cdev, const char *buf, size_t nbyte, uint64_t pos)
{
    static uint8_t block[512];

    int bytes_this_block;
    int bytes_written;
    int blocks_required;
    int i;
    int start_sector;
    int start_offset;
 
    struct device *dev;
    
    dev = cdev->state;

    bytes_written = 0;
    blocks_required = (nbyte >> 9) + 1;
    start_sector = pos >> 9;
    start_offset = pos & 0x1FF;
    
    for (i = 0; i < blocks_required; i++) {
        bytes_this_block = MIN(512-start_offset, nbyte - bytes_written);

        vblk_read_sector(dev, start_sector + i, block);
        memcpy(&block[start_offset], &buf[bytes_written], bytes_this_block);
        vblk_write_sector(dev, start_sector + i, block);

        start_offset = 0;
        bytes_written += bytes_this_block;
    }

    return nbyte;
}

static int
vblk_attach(struct driver *driver, struct device *dev)
{
    static int vd_counter = 0;

    int res;

    char cdev_name[16];

    struct cdev_ops cdev_ops;
    struct cdev *cdev;

    res = virtio_attach(dev);

    if (res != 0) {
        return res;
    }

    sprintf(cdev_name, "rvd%c", '0' + vd_counter);

    cdev_ops = (struct cdev_ops) {
        .close  = NULL,
        .init   = NULL,
        .ioctl  = vblk_ioctl,
        .isatty = NULL,
        .mmap   = NULL,
        .open   = NULL,
        .read   = vblk_read,
        .write  = vblk_write
    };

    cdev = cdev_new(cdev_name, 0666, DEV_MAJOR_RAW_DISK, vd_counter++, &cdev_ops, dev);

    if (cdev && cdev_register(cdev) == 0) {
        return 0;
    }

    return -1;
}

static int
vblk_deattach(struct driver *driver, struct device *dev)
{
    return 0;
}

static int
vblk_probe(struct driver *driver, struct device *dev)
{
    if (pci_get_vendor_id(dev) == 0x1AF4 && (pci_get_device_id(dev) >= 0x1000 || pci_get_device_id(dev) < 0x103F)) {
        if (pci_get_subsystem_id(dev) == 0x02) return 0;
    }

    return -(ENXIO);
}
