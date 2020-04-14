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
    uint8_t     data[512];
    uint8_t     status;
} __attribute__((packed));

struct vblk_dev {
    struct device   *   dev;

};

struct driver virtio_blk_driver = {
    .attach     =   vblk_attach,
    .deattach   =   vblk_deattach,
    .probe      =   vblk_probe,
};

static int
vblk_write_sector(struct device *dev, int sector, void *data)
{
    static struct virtio_blk_req req;
    memset(&req, 0, sizeof(req));
    memcpy(req.data, data, 512);
    req.type = VIRTIO_BLK_T_OUT;
    req.status = 0;
    req.reserved = 0;
    req.sector = sector;

    struct virtq_buffer buffers[] = {
        {
            .length = 528,
            .flags = VIRTQ_DESC_F_NEXT,
            .buf = &req
        },
        {
            .length = 1,
            .flags = VIRTQ_DESC_F_WRITE,
            .buf = &req.status
        }
    };

    virtq_send(dev, 0, buffers, 2);
    return 0;
}
/*
static int
vblk_read_sector(struct device *dev, int sector, void *data)
{
	static struct virtio_blk_req req;
    memset(&req, 0, sizeof(req));
    req.type = VIRTIO_BLK_T_IN;
    req.status = 0;
    req.reserved = 0;
    req.sector = sector;

	uint8_t status;
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

	return 0;
}*/

static int
vblk_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    return 0;
}

static int
vblk_write(struct cdev *cdev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct device *dev = cdev->state;

    uint8_t block[512];

    memcpy(block, buf, nbyte);

    vblk_write_sector(dev, 0, block);

    return 0;
}

static int
vblk_attach(struct driver *driver, struct device *dev)
{
    static int vd_counter = 0;

    int res = virtio_attach(dev);

    if (res != 0) {
        return res;
    }
    struct vblk_dev *vblk = calloc(1, sizeof(struct vblk_dev));
    vblk->dev = dev;

    struct cdev *vd = calloc(1, sizeof(struct cdev) + 14);
   
    char *name = (char*)&vd[1];
    sprintf(name, "rvd%c", 'a' + vd_counter);

    vd->name = name;
    vd->mode = 0600;
    vd->majorno = DEV_MAJOR_RAW_DISK;
    vd->minorno = vd_counter;
    vd->state = vblk;
    vd->read = vblk_read;
    vd->write = vblk_write;
    
    vd_counter++;

    cdev_register(vd);
    printf("virtio_blk: initialized\n\r");
    return 0;
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
