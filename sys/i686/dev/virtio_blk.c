#include <machine/pci.h>
#include <machine/portio.h>
#include <machine/vm.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include "virtio.h"

#define MIN(X, Y) ((X < Y) ? X : Y)

#define VIRTIO_BLK_T_IN           0 
#define VIRTIO_BLK_T_OUT          1 
#define VIRTIO_BLK_T_FLUSH        4 

static int vblk_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
static int vblk_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);
static int vblk_probe(struct driver *driver, struct device *dev);
static int vblk_attach(struct driver *driver, struct device *dev);
static int vblk_deattach(struct driver *driver, struct device *dev);

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
vblk_write_sector(struct device *dev, int sector, void *data)
{
    uint8_t status = 0xFF;
    struct virtio_blk_req req = {
        .type = VIRTIO_BLK_T_OUT,
        .sector = sector,
        .reserved = 0
    };

    struct virtq_buffer buffers[] = {
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

    if (status != 0) {
        printf("virtio_blk: write_sector() error %d\n\r", status);
        return -1;
    }

    return 0;
}

static int
vblk_read_sector(struct device *dev, int sector, void *data)
{
    uint8_t status = 0xFF;
	struct virtio_blk_req req = {
        .type = VIRTIO_BLK_T_IN,
        .sector = sector,
        .reserved = 0
    };

    struct virtq_buffer buffers[] = {
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

    if (status != 0) {
        printf("virtio_blk: read_sector() error %d\n\r", status);
        return -1;
    }
	return 0;
}

static int
vblk_read(struct cdev *cdev, char *buf, size_t nbyte, uint64_t pos)
{
    static uint8_t block[512];
    struct device *dev = cdev->state;

    int bytes_read = 0;
    int blocks_required = (nbyte >> 9) + 1;
    int start_sector = pos >> 9;
    int start_offset = pos & 0x1FF;

    for (int i = 0; i < blocks_required; i++) {
        int bytes_this_block = MIN(512, nbyte - bytes_read);
        
        vblk_read_sector(dev, start_sector + i, block);
        memcpy(&buf[i<<9], &block[start_offset], bytes_this_block);

        start_offset = 0;
        bytes_read += bytes_this_block;
    }

    return 0;
}

static int
vblk_write(struct cdev *cdev, const char *buf, size_t nbyte, uint64_t pos)
{
    static uint8_t block[512];
    struct device *dev = cdev->state;

    int bytes_written = 0;
    int blocks_required = (nbyte >> 9) + 1;
    int start_sector = pos >> 9;
    int start_offset = pos & 0x1FF;
    
    for (int i = 0; i < blocks_required; i++) {
        int bytes_this_block = MIN(512, nbyte - bytes_written);
        
        vblk_read_sector(dev, start_sector + i, block);
        memcpy(&block[start_offset], &buf[i<<9], bytes_this_block);
        vblk_write_sector(dev, start_sector + i, block);

        start_offset = 0;
        bytes_written += bytes_this_block;
    }

    memcpy(block, buf, nbyte);

    return nbyte;
}

static int
vblk_attach(struct driver *driver, struct device *dev)
{
    static int vd_counter = 0;

    int res = virtio_attach(dev);

    if (res != 0) {
        return res;
    }

	char cdev_name[16];
    sprintf(cdev_name, "vd%c", 'a' + vd_counter);

	struct cdev_ops cdev_ops = {
        .close  = NULL,
        .init   = NULL,
        .ioctl  = NULL,
        .isatty = NULL,
        .mmap   = NULL,
        .open   = NULL,
        .read   = vblk_read,
        .write  = vblk_write
    };

    struct cdev *cdev = cdev_new(cdev_name, 0666, DEV_MAJOR_RAW_DISK, vd_counter++, &cdev_ops, dev);

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

    return -1;
}
