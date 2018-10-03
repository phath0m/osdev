; defined in sys/i686/sched.c
extern sched_curr_page_dir
extern sched_get_next_proc

global sched_switch_context

global biz_baz
biz_baz:

    cli
    hlt

sched_switch_context:
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
    iret
