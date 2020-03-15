/*
 * bus.h
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
#ifndef _SYS_BUS_H
#define _SYS_BUS_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <machine/bus.h>

struct regs;

typedef int (*intr_handler_t)(int inum, struct regs *regs);

int bus_register_intr(int inum, intr_handler_t handler);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _SYS_BUS_H */
