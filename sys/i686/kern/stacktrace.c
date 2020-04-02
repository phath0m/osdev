/*
 * stacktrace.c - Debug function to print a stacktrace
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
#include <sys/systm.h>
#include <sys/types.h>

struct stackframe {
    struct stackframe * prev;
    uintptr_t           eip;
};

void
stacktrace(int max_frames)
{
	char name[256];
	uintptr_t offset;
    struct stackframe *frame;

    asm volatile("movl %%ebp, %%edx": "=d"(frame));
    printf("Trace:\n\r");

    for (int i = 0; i < max_frames && frame; i++) {
        if (ksym_find_nearest(frame->eip, &offset, name, 256) == 0) {
            printf("    [0x%p] <%s+0x%x>\n\r", frame->eip, name, offset);
        } else {
            printf("    [0x%p]\n\r", frame->eip);
        }

        frame = frame->prev;
    }
}
