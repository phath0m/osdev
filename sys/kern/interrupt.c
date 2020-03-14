/*
 * interrupt.c - Implements functionality for registering interrupt handlers
 *
 * Note: This file will probably be axed in future refactoring efforts
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
#include <sys/bus.h>

intr_handler_t intr_handlers[256];

void
dispatch_to_intr_handler(int inum, struct regs *regs)
{
    intr_handler_t handler = intr_handlers[inum];

    if (handler) {
        handler(inum, regs);
    }
}

int
bus_register_intr(int inum, intr_handler_t handler)
{
    intr_handlers[inum] = handler;
    
    return 0;
}
