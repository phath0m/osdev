MBALIGN     equ 1<<0
MEMINFO     equ 1<<1
FLAGS       equ MBALIGN | MEMINFO

MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)

section .data
align 0x1000
boot_page_directory:
    dd 0x00000083
    times (KERNEL_PAGE_NUMBER - 1) dd 0
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUMBER - 1) dd 0

section .text

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

global _start

_start:
    mov ecx, (boot_page_directory - KERNEL_VIRTUAL_BASE)
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x00000010
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    lea ecx, [_start_virtual]
    jmp ecx

    ; move this
    mov esp, stack_top
    extern _preinit
    extern start_bss
    extern end_bss
    push ebx


    call _preinit
    cli
    jmp $

_start_virtual:
    mov dword [boot_page_directory], 0
    invlpg [0]
    mov esp, stack_top
    push eax
    add ebx, KERNEL_VIRTUAL_BASE
    push ebx
    ; clear .bss before calling _preinit
    mov ebx, start_bss
loop:
    mov [ebx], byte 0
    inc ebx
    cmp ebx, end_bss
    jb loop

    call _preinit
    cli
    hlt
    jmp $

section .bss
align 4

section .bootstrap_stack
stack_bottom:
times 16386 db 0

stack_top:

