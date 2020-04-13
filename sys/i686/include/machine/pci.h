/*
 * pci.h
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
#ifndef _MACHINE_PCI_H
#define _MACHINE_PCI_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <sys/device.h>
#include <sys/types.h>

#define PCI_CONFIG_BAR0		0x10
#define PCI_CONFIG_BAR1		0x14
#define PCI_CONFIG_BAR2		0x18
#define PCI_CONFIG_BAR3		0x1C
#define PCI_CONFIG_BAR4		0x20
#define PCI_CONFIG_BAR5		0x24
#define PCI_CONFIG_IRQLINE  0x3C

#define PCI_IO_BASE(bar)    ((bar) & 0xFFFFFFFC)

int     pci_config_read(struct device *dev, int offset);
int     pci_get_device_id(struct device *dev);
int     pci_get_vendor_id(struct device *dev);
int     pci_get_class(struct device *dev);
int     pci_get_subclass(struct device *dev);
int     pci_get_subsystem_id(struct device *dev);

uint8_t     pci_get_config8(struct device *dev, int offset);
uint16_t 	pci_get_config16(struct device *dev, int offset);
uint32_t	pci_get_config32(struct device *dev, int offset);

void    pci_init();

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _MACHINE_PCI_H */
