#include "timer.h"
#include "idt.h"
#include "io.h"
#include "serial.h"
#include "scheduler.h"

uint32_t tick = 0;

static void timer_callback(registers_t* regs) {
    (void)regs;
    tick++;
    if (tick % 10 == 0) {
        if (get_current_thread() != 0) {
            schedule();
        }
    }
}

void timer_init(uint32_t frequency) {
    register_interrupt_handler(32, &timer_callback);
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
}

uint32_t timer_get_ticks(void) {
    return tick;
}
