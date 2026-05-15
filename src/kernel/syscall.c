#include "syscall.h"
#include "serial.h"
#include "scheduler.h"

static void sys_print(const char* str) {
    serial_printf("[USER] %s", str);
}

void syscall_handler(registers_t* regs) {
    switch (regs->eax) {
        case 1: // sys_print
            sys_print((const char*)regs->ebx);
            break;
        case 2: // sys_yield
            scheduler_yield();
            break;
        default:
            serial_printf("Unknown syscall: %d\n", regs->eax);
            break;
    }
}

void syscall_init(void) {
    register_interrupt_handler(128, syscall_handler);
}
