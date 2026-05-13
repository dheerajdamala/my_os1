#include "kernel.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "memory.h"
#include "scheduler.h"
#include "ipc.h"
#include "ktf.h"
#include "vga.h"
#include "splash.h"
#include "dashboard.h"
#include "tty.h"
#include "keyboard.h"
#include "shell.h"
#include "paging.h"

/* ── Background worker thread (keeps IPC busy for the dashboard stats) ── */
static void worker_thread(void) {
    thread_t* self = get_current_thread();
    int counter = 0;
    while (1) {
        ipc_message_t msg;
        msg.sender_id    = self->id;
        msg.message_type = 1;
        msg.data1        = counter++;
        msg.data2        = 0;
        ipc_send(1, &msg);
        for (volatile int i = 0; i < 8000000; i++);
        scheduler_yield();
    }
}

/* ── Receiver thread ──────────────────────────────────────────────────── */
static void receiver_thread(void) {
    thread_t* self = get_current_thread();
    while (1) {
        ipc_message_t msg;
        ipc_receive(self->id, &msg);
        for (volatile int i = 0; i < 5000000; i++);
        scheduler_yield();
    }
}

/* ── Kernel entry point ───────────────────────────────────────────────── */
void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;

    /* 1. Serial first (debugging before VGA is up) */
    serial_init();

    /* 2. GDT must precede paging and IDT */
    gdt_init();

    /* 3. Enable paging (identity maps 0–4 MB, transparent to existing code) */
    paging_init();

    /* 4. IDT + page-fault handler */
    idt_init();
    register_interrupt_handler(14, paging_fault_handler);

    /* 5. VGA + splash screen */
    vga_init();
    splash_show();

    if (magic != 0x2BADB002) return;

    /* 6. Kernel subsystems */
    ktf_init();
    timer_init(100);
    pmm_init(128 * 1024 * 1024);
    kheap_init();

    /* 7. Enable interrupts */
    asm volatile("sti");

    /* 8. Keyboard (must be after sti so IRQ1 can fire) */
    keyboard_init();

    /* 9. Scheduler + IPC */
    scheduler_init();
    ipc_init();

    /* 10. Draw static dashboard chrome */
    dashboard_init();

    /* 11. Init TTY (writes welcome banner into the TTY region) */
    tty_init();

    /* 12. Spawn threads:
     *   Thread 1 = receiver  (provides IPC target for worker)
     *   Thread 2 = worker    (sends IPC messages, keeps stats live)
     *   Thread 3 = shell     (interactive command interpreter)    */
    thread_create(receiver_thread);
    thread_create(worker_thread);
    thread_create(shell_thread);

    /* 13. Hand off to the scheduler */
    schedule();

    while (1) { asm volatile("hlt"); }
}
