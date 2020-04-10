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

int     pci_config_read(struct device *dev, int offset);
int     pci_get_device_id(struct device *dev);
int     pci_get_vendor_id(struct device *dev);
void    pci_init();

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _MACHINE_PCI_H */
