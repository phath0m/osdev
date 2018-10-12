global frame_copy

frame_copy:
    push ebp
    mov ebp, esp
    
    push edi
    push esi

    mov edi, [ebp+0x0C]
    mov eax, [ebp+0x08]
    
    mov edx, cr3
    mov cr3, eax
    cld
    
    rep movsb
      
    mov cr3, edx
    mov edx, cr3
    mov cr3, edx 
  
    pop esi
    pop edi
    pop ebp
    
    ret
