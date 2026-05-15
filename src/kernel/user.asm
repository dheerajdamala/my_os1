[bits 32]
global enter_user_mode
enter_user_mode:
    ; arg1: function pointer
    mov ebx, [esp + 4]

    cli
    mov ax, 0x23 ; User data selector (0x20 | 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Push the stack frame for iret
    push 0x23        ; SS
    push esp         ; ESP (current stack is fine for this demo)
    pushfd           ; EFLAGS
    pop eax
    or eax, 0x200    ; Enable IF (Interrupt Flag)
    push eax         ; Pushed EFLAGS
    push 0x1B        ; CS (0x18 | 3)
    push ebx         ; EIP (target function)
    
    iret
