MBALIGN     equ  1 << 0
MEMINFO     equ  1 << 1
VIDEO_MODE  equ  1 << 2
FLAGS       equ  MBALIGN | MEMINFO | VIDEO_MODE
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    ; Address placeholders (required to align graphics fields at offset 32)
    dd 0            ; header_addr
    dd 0            ; load_addr
    dd 0            ; load_end_addr
    dd 0            ; bss_end_addr
    dd 0            ; entry_addr
    ; Graphics fields (offset 32)
    dd 0            ; mode_type (0 = linear graphics)
    dd 0            ; width (0 = no preference)
    dd 0            ; height (0 = no preference)
    dd 0            ; depth (0 = no preference)


section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global start
extern kernel_main

start:
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
