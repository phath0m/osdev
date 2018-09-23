MBALIGN     equ 1<<0
MEMINFO     equ 1<<1
FLAGS       equ MBALIGN | MEMINFO

MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot

align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000

section .bootstrap_stack
align 4

stack_bottom:
times 16386 db 0

stack_top:

section .text

global _start

_start:
    mov esp, stack_top
    extern _preinit
    extern start_bss
    extern end_bss
    push ebx

    mov ebx, start_bss
loop:
    mov [ebx], byte 0
    inc ebx
    cmp ebx, end_bss
    jb loop

    call _preinit
    cli
    jmp $
