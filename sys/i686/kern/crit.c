/*
 * crit.c - prevents kernel level code from being pre-empted
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
#include <sys/types.h>
#include <sys/mutex.h>

static inline uint32_t
get_eflags()
{
    uint32_t flags;

    asm volatile("pushf ; pop %0" : "=rm" (flags) :: "memory");

    return flags;
}

static bool prev_if;
static spinlock_t crit_lock = 0;

void
critical_enter()
{
    prev_if = (get_eflags() & 0x200) != 0;

    if (prev_if) {
        asm volatile("cli");
    }

    spinlock_lock(&crit_lock);
}

void
critical_exit()
{
    spinlock_unlock(&crit_lock);

    if (prev_if) {
        asm volatile("sti");
    }
}
