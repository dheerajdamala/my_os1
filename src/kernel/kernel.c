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
#include "vfs.h"
#include "ramfs.h"
#include "syscall.h"
#include "ata.h"
#include "fat32.h"

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

/* ── User Mode Test Thread ────────────────────────────────────────────── */
extern void enter_user_mode(void (*fn)(void));

extern uint32_t elf_load_file(const char* path);

static void user_thread_entry(void) {
    thread_t* self = get_current_thread();
    set_kernel_stack(self->stack_base + 4096);

    serial_printf("[KERNEL] Attempting to load disk/test.elf...\n");
    uint32_t entry = elf_load_file("disk/test.elf");
    if (entry) {
        serial_printf("[KERNEL] ELF Loaded at 0x%x. Transitioning to User Mode...\n", entry);
        enter_user_mode((void (*)(void))entry);
    } else {
        serial_printf("[KERNEL] Failed to load ELF!\n");
    }
}

/* ── Kernel entry point ───────────────────────────────────────────────── */
void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;

    /* 1. Serial first (debugging before VGA is up) */
    serial_init();
    serial_printf("SENTINEL OS BOOTING...\n");

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

    /* Initialize VFS and RamFS */
    vfs_init();
    ramfs_init();

    /* Initialize ATA and FAT32 */
    ata_init();
    fat32_init();
    vfs_node_t* fat_root = fat32_get_root_node();
    if (fat_root) {
        ramfs_create_mountpoint("disk", fat_root);
    }

    /* 6. Kernel subsystems */
    ktf_init();
    timer_init(100);
    pmm_init(128 * 1024 * 1024);
    kheap_init();

    /* Verify dynamic kernel heap allocator */
    serial_printf("[KERNEL] Heap used initially: %d bytes\n", kheap_used());
    void* k1 = kmalloc(100);
    void* k2 = kmalloc(200);
    serial_printf("[KERNEL] Allocated 100 bytes at 0x%x and 200 bytes at 0x%x. Heap used: %d bytes\n", k1, k2, kheap_used());
    kfree(k1);
    kfree(k2);
    serial_printf("[KERNEL] Freed allocations. Heap used: %d bytes\n", kheap_used());

    /* 7. Enable interrupts */
    asm volatile("sti");

    /* 8. Keyboard (must be after sti so IRQ1 can fire) */
    keyboard_init();

    /* 9. Scheduler + IPC + Syscalls */
    scheduler_init();
    ipc_init();
    syscall_init();

    /* 10. Draw static dashboard chrome */
    dashboard_init();

    /* 11. Init TTY (writes welcome banner into the TTY region) */
    tty_init();

    /* 12. Spawn threads:
     *   Thread 1 = receiver  (provides IPC target for worker)
     *   Thread 2 = worker    (sends IPC messages, keeps stats live)
     *   Thread 3 = shell     (interactive command interpreter)
     *   Thread 4 = user mode test thread */
    thread_create(receiver_thread);
    thread_create(worker_thread);
    thread_create(shell_thread);
    thread_create(user_thread_entry);

    /* 13. Hand off to the scheduler */
    schedule();

    while (1) { asm volatile("hlt"); }
}
