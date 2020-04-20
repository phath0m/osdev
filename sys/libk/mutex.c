/*
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
#include <sys/mutex.h>
#include <sys/systm.h>
#include <machine/interrupt.h>

static inline uint32_t
get_eflags()
{
    uint32_t flags;

    asm volatile("pushf ; pop %0" : "=rm" (flags) :: "memory");

    return flags;
}

void
spinlock_lock(spinlock_t volatile *lock)
{
    /* debug code to detect deadlocks. will remove */
    uint32_t flags = get_eflags();
    
    bool deadlock =  (*lock && (flags & 0x200) == 0);

    if (deadlock) {
        printf("kernel: we will deadlock\n\r");
        asm volatile("sti");
    }

    while (*lock) {
    }
    
    if (deadlock) {
        asm volatile("cli");
    }

    __sync_lock_test_and_set(lock, 1);
}

void
spinlock_unlock(spinlock_t volatile *lock)
{
    __sync_lock_release(lock, 0);
}
