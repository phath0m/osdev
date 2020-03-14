; usermode.asm - Return from kernel space to usermode
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

global return_to_usermode

return_to_usermode:
    push ebp
    mov ebp, esp
    mov ebx, [ebp + 0x08]
    mov ecx, [ebp + 0x0C]
    mov edx, [ebp + 0x10]
    mov edi, [ebp + 0x14]     
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
    mov ebp, edx
    mov eax, edi
    sti
    iret
