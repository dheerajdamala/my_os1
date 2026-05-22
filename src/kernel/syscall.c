#include "syscall.h"
#include "serial.h"
#include "scheduler.h"

uint32_t user_brk_start = 0x2000000; // 32 MB
uint32_t user_brk_current = 0x2000000;

static void sys_print(const char* str) {
    serial_printf("[USER] %s", str);
}

static uint32_t sys_brk(uint32_t new_brk) {
    if (new_brk == 0) {
        return user_brk_current;
    }
    if (new_brk < user_brk_start) {
        return user_brk_current;
    }
    if (new_brk > 0x4000000) { // Limit heap to 64 MB
        serial_printf("[SYSCALL] sys_brk: Out of heap memory limit! (0x%x)\n", new_brk);
        return user_brk_current;
    }
    uint32_t old_brk = user_brk_current;
    user_brk_current = new_brk;
    serial_printf("[SYSCALL] sys_brk: Expanded heap break from 0x%x to 0x%x\n", old_brk, new_brk);
    return user_brk_current;
}

void syscall_handler(registers_t* regs) {
    switch (regs->eax) {
        case 1: // sys_print
            sys_print((const char*)regs->ebx);
            break;
        case 2: // sys_yield
            scheduler_yield();
            break;
        case 3: // sys_brk
            regs->eax = sys_brk(regs->ebx);
            break;
        default:
            serial_printf("Unknown syscall: %d\n", regs->eax);
            break;
    }
}

void syscall_init(void) {
    register_interrupt_handler(128, syscall_handler);
}
