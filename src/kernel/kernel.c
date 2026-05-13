#include "kernel.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "memory.h"
#include "scheduler.h"
#include "ipc.h"
#include "ktf.h"

void thread1_main(void) {
    thread_t* self = get_current_thread();
    while(1) {
        ipc_message_t msg;
        if (ipc_receive(self->id, &msg) == 0) {
            // Received message
        }
        for(volatile int i=0; i<10000000; i++);
        scheduler_yield();
    }
}

void thread2_main(void) {
    thread_t* self = get_current_thread();
    int counter = 0;
    while(1) {
        ipc_message_t msg;
        msg.sender_id = self->id;
        msg.message_type = 1;
        msg.data1 = counter++;
        msg.data2 = 0;
        ipc_send(1, &msg);
        for(volatile int i=0; i<10000000; i++);
        scheduler_yield();
    }
}

void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;

    serial_init();
    gdt_init();
    idt_init();

    if (magic != 0x2BADB002) return;

    ktf_init();
    timer_init(100);

    pmm_init(128 * 1024 * 1024);
    kheap_init();

    asm volatile("sti");

    scheduler_init();
    ipc_init();

    thread_create(thread1_main);
    thread_create(thread2_main);

    schedule();

    while (1) { asm volatile("hlt"); }
}
