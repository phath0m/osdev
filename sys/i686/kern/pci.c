#include <machine/portio.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/types.h>

/*
const char *class_lut[] = {
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge",
    "Simple Communciation Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller"
};
*/

struct pci_device {
    struct device   _header;
    uint8_t         bus;
    uint8_t         slot;
    uint8_t         func;
    uint16_t        device_id;
    uint16_t        vendor_id;
    uint8_t         class;
    uint8_t         header_type;
};

uint16_t
pci_read_short(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;

    address = (uint32_t)((bus << 16) | (slot << 11) |
            (func << 8) | (offset & 0xfC) | 0x80000000);

    io_write_long(0xCF8, address);
    
    return (io_read_long(0xCFC) >> ((offset & 2) << 3) & 0xFFFF);
}

int
pci_get_device_id(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pcidev->device_id;
}

int
pci_get_vendor_id(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pcidev->vendor_id;
}

bool
pci_enumerate_device(uint8_t bus, uint8_t slot, uint8_t func)
{
    uint16_t vendor = pci_read_short(bus, slot, 0, 0);

    if (vendor == 0xFFFF) {
        return false;
    }

    struct pci_device *dev = calloc(1, sizeof(struct pci_device));

    dev->_header.type = DEVICE_PCI;
    dev->vendor_id = vendor;
    dev->device_id = pci_read_short(bus, slot, 0, 2);
    dev->class  = pci_read_short(bus, slot, 0, 0x0A) >> 8;
    dev->header_type = pci_read_short(bus, slot, 0, 0x0E) & 0xFF;

    printf("PCI: %d:%d.%d\n\r", bus, slot, func);

    device_register((struct device*)dev);

    if (func == 0 && (dev->header_type & 0x80)) {
        for (int i = 1; i < 8; i++) { 
            pci_enumerate_device(bus, slot, i);
        }
    }

    return true;
}

void
pci_init()
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            if (!pci_enumerate_device(bus, slot, 0)) continue;
        }
    }
}
