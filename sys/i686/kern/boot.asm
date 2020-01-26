MBALIGN     equ 1<<0
MEMINFO     equ 1<<1
USEGFX      equ 1<<2
FLAGS       equ MBALIGN | MEMINFO | USEGFX

MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)

global _start
global boot_page_directory

section .data
align 0x1000
boot_page_directory:
    dd 0x00000083
    times (KERNEL_PAGE_NUMBER - 1) dd 0
    dd 0x00000083
    dd 0x00400083
    dd 0x00800083
    dd 0x00C00083
    dd 0x01000083
    dd 0x01400083
    dd 0x01800083
    dd 0x01C00083
    dd 0x02000083
    dd 0x02400083
    dd 0x02800083
    dd 0x02C00083
    dd 0x03000083
    dd 0x03400083
    dd 0x03800083
    dd 0x03C00083
    dd 0x04000083
    dd 0x04400083
    dd 0x04800083
    dd 0x04C00083
    dd 0x05000083
    dd 0x05400083
    dd 0x05800083
    dd 0x05C00083
    dd 0x06000083
    dd 0x06400083
    dd 0x06800083
    dd 0x06C00083
    dd 0x07000083
    dd 0x07400083
    dd 0x07800083
    dd 0x07C00083
    dd 0x08000083
    dd 0x08400083
    dd 0x08800083
    dd 0x08C00083
    dd 0x09000083
    dd 0x09400083
    dd 0x09800083
    dd 0x09C00083
    dd 0x0A000083
    dd 0x0A400083
    dd 0x0A800083
    dd 0x0AC00083
    dd 0x0B000083
    dd 0x0B400083
    dd 0x0B800083
    dd 0x0BC00083
    dd 0x0C000083
    dd 0x0C400083
    dd 0x0C800083
    dd 0x0CC00083
    dd 0x0D000083
    dd 0x0D400083
    dd 0x0D800083
    dd 0x0DC00083
    dd 0x0E000083
    dd 0x0E400083
    dd 0x0E800083
    dd 0x0EC00083
    dd 0x0F000083
    dd 0x0F400083
    dd 0x0F800083
    dd 0x0FC00083
    times (1024 - KERNEL_PAGE_NUMBER - 64) dd 0

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

section .text
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


