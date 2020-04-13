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
	uint16_t		subsystem_id;
    uint8_t         class;
    uint8_t         subclass;
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

uint32_t
pci_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	return (pci_read_short(bus, slot, func, offset + 2) << 16) | pci_read_short(bus, slot, func, offset);
}


void
pci_write_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data)
{
    uint32_t address;

    address = (uint32_t)((bus << 16) | (slot << 11) |
            (func << 8) | (offset & 0xfC) | 0x80000000);

    io_write_long(0xCF8, address);
	io_write_long(0xCFC, data);
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

int
pci_get_class(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;
    
    return pcidev->class;
}

uintptr_t
pci_get_bar0(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pci_read_long(pcidev->bus, pcidev->slot, pcidev->func, 0x10);
}

int
pci_get_subclass(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pcidev->subclass;
}

int
pci_get_subsystem_id(struct device *dev)
{
    if (dev->type != DEVICE_PCI) {
        return -1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pcidev->subsystem_id;
}

uint8_t
pci_get_config8(struct device *dev, int offset)
{
    if (dev->type != DEVICE_PCI) {
        return (uint8_t)-1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pci_read_short(pcidev->bus, pcidev->slot, pcidev->func, offset) & 0xFF;
}

uint16_t
pci_get_config16(struct device *dev, int offset)
{
	if (dev->type != DEVICE_PCI) {
		return (uint16_t)-1;
	}
    
    struct pci_device *pcidev = (struct pci_device*)dev;

    return pci_read_short(pcidev->bus, pcidev->slot, pcidev->func, offset);
}

uint32_t
pci_get_config32(struct device *dev, int offset)
{
    if (dev->type != DEVICE_PCI) {
        return (uint32_t)-1;
    }

    struct pci_device *pcidev = (struct pci_device*)dev;

    return pci_read_long(pcidev->bus, pcidev->slot, pcidev->func, offset);
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
	dev->bus = bus;
	dev->slot = slot;
	dev->func = func;
    dev->vendor_id = vendor;
    dev->device_id = pci_read_short(bus, slot, func, 2);
    dev->class  = pci_read_short(bus, slot, func, 0x0A) >> 8;
    dev->subclass = pci_read_short(bus, slot, func, 0x0A) & 0xFF;
    dev->header_type = pci_read_short(bus, slot, func, 0x0E) & 0xFF;
	dev->subsystem_id = pci_read_short(bus, slot, func, 0x2E);
    printf("PCI: %d:%d.%d\n\r", bus, slot, func);

    device_register((struct device*)dev);

    if (func == 0 && (dev->header_type & 0x80) != 0) {
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
