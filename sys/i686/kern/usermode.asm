global return_to_usermode

return_to_usermode:
    push ebp
    mov ebp, esp
    mov ebx, [ebp + 0x08]
    mov ecx, [ebp + 0x0C]
     
    mov ax,0x23
    mov ds,ax
    mov es,ax 
    mov fs,ax 
    mov gs,ax

    push 0x23
    push ecx
    pushf
    push 0x1B;
    push ebx
    mov eax, 10
    iret
