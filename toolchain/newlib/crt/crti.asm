section .init
global _init

_init:
    push ebp
    ; constructors will go here...

section .fini
global _fini
_fini:
    push ebp
    ; deconstructors will go here...
