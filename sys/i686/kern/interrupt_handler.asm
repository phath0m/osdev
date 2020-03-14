; interrupt_handler.asm - x86 interrupt handlers 
;
; This file defines the actual interrupt handlers. This is done in assembly to
; allow more granular control over the stack and to setup the struct regs *
; argument that's actually passed to the C portion of the interrupt dispatcher
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

%macro ISR_NOERRCODE 1
    global isr%1
    
    isr%1:
        push byte 0
        push byte %1
        jmp isr_handler
%endmacro

%macro ISR_ERRCODE 1
    global isr%1

    isr%1:
        push byte %1
        jmp isr_handler
%endmacro

%macro IRQ 2
    global irq%1

    irq%1:
        push byte 0
        push byte %2
        jmp irq_handler
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_NOERRCODE 8
ISR_NOERRCODE 9
ISR_NOERRCODE 10
ISR_NOERRCODE 11
ISR_NOERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

ISR_NOERRCODE 128
; defined in i686/kern/context_switch.asm
extern sched_switch_context

global irq0
; this is special and cannot be overwritten. 
irq0:
    jmp sched_switch_context

IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

global isr_handler
global irq_handler

extern dispatch_intr

isr_handler:
    cli
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp

    call dispatch_intr

    add esp, 4

    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 8

    sti
    iret

irq_handler:
   cli
   pusha

   mov ax, ds
   
   push eax

   mov ax, 0x10
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax

   push esp

   call dispatch_intr

   add esp, 4

   pop ebx

   mov ds, bx
   mov es, bx
   mov fs, bx
   mov gs, bx

   popa

   add esp, 8
   
   sti
   iret
