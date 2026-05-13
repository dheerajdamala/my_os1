global switch_to_thread
switch_to_thread:
    push ebp
    push edi
    push esi
    push ebx
    mov eax, [esp + 20]
    mov [eax], esp
    mov eax, [esp + 24]
    mov esp, [eax]
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret
