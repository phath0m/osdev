global _start

extern _premain

_start:
    mov eax, [esp + 0x04] ; int argc
    mov ebx, [esp + 0x08] ; const char *argp[]
    mov ecx, [esp + 0x0C] ; int envc
    mov edx, [esp + 0x10] ; const char *envp[]

    push edx
    push ecx
    push ebx
    push eax

    call _premain

    mov ebx, eax
    mov eax, 0x0D

    int 0x80

    jmp $
