/*
 * interrupt.h
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
#ifndef _ELYSIUM_SYS_INTERRUPT_H
#define _ELYSIUM_SYS_INTERRUPT_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <machine/interrupt.h>

struct device;
struct regs;

/* software interrupt (trap) */
typedef int (*intr_handler_t)(int, struct regs *);

/* device interrupt handler (IRQs) */
typedef int (*dev_intr_t)(struct device *, int);

int swi_register(int, intr_handler_t);
int	irq_register(struct device *, int, dev_intr_t);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _ELYSIUM_SYS_INTERRUPT_H */
