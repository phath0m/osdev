; context_switch.asm - Performs a context switch
;
; The way the scheduler works is by pushing the process's state onto the stack.
; The current stack pointer to the process's state is passed to sched_get_next_proc()
; sched_get_next_proc() returns to the new stackpointer to the new process. This
; will then set ESP to the returned value from sched_get_next_proc(), which will
; cause the state of that process to be popped from the stack back into their
; registers.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along
; with this program; if not, write to the Free Software Foundation, Inc.,
; 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

; defined in sys/i686/sched.c
extern sched_curr_page_dir
extern sched_get_next_proc

global sched_switch_context

sched_switch_context:
    cli
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebx, cr3
    mov eax, esp

    push eax

    call sched_get_next_proc

    mov esp, eax

    mov eax, [sched_curr_page_dir]
    cmp eax, 0

    jz .no_page_dir
    mov cr3, eax
    mov eax, cr3
    mov cr3, eax

.no_page_dir:
    mov al, 0x20
    out 0x20, al

    pop gs
    pop fs
    pop es
    pop ds
    popad
    sti
    iret
