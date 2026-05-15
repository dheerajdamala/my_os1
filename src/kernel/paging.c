#include "paging.h"
#include "tty.h"

/*
 * SentinelOS — x86 Identity Paging (Milestone 2)
 * ───────────────────────────────────────────────
 * Sets up a minimal page directory that identity-maps the first 4 MB of
 * physical memory (virtual address == physical address).  This keeps all
 * existing kernel code, VGA buffer, and stack working transparently while
 * enabling the MMU for future memory protection.
 *
 * Page directory / table format (32-bit non-PAE):
 *   Bits 31-12 : physical base address of next level / page frame
 *   Bit  2     : U/S  (0 = supervisor only)
 *   Bit  1     : R/W  (1 = read+write)
 *   Bit  0     : P    (1 = present)
 */

#define PAGE_PRESENT  (1 << 0)
#define PAGE_RW       (1 << 1)
#define PAGE_USER     (1 << 2)
#define PAGE_SIZE_4K  4096
#define PAGES_PER_TAB 1024

/* Statically allocated, 4 KB aligned structures.
 * __attribute__((aligned(4096))) ensures the linker puts them on a page
 * boundary so we can load their address directly into CR3.               */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table_0[1024]   __attribute__((aligned(4096))); /* 0  -  4 MB */
static uint32_t page_table_1[1024]   __attribute__((aligned(4096))); /* 4  -  8 MB */
static uint32_t page_table_2[1024]   __attribute__((aligned(4096))); /* 8  - 12 MB */
static uint32_t page_table_3[1024]   __attribute__((aligned(4096))); /* 12 - 16 MB */

void paging_init(void) {
    /* 1. Zero the page directory */
    for (int i = 0; i < 1024; i++) page_directory[i] = 0;

    /* 2. Identity-map 0–16 MB across 4 page tables (4096 pages total) */
    for (int i = 0; i < PAGES_PER_TAB; i++) {
        uint32_t flags = PAGE_PRESENT | PAGE_RW | PAGE_USER;
        page_table_0[i] = ((0 * PAGES_PER_TAB + i) * PAGE_SIZE_4K) | flags;
        page_table_1[i] = ((1 * PAGES_PER_TAB + i) * PAGE_SIZE_4K) | flags;
        page_table_2[i] = ((2 * PAGES_PER_TAB + i) * PAGE_SIZE_4K) | flags;
        page_table_3[i] = ((3 * PAGES_PER_TAB + i) * PAGE_SIZE_4K) | flags;
    }

    /* 3. Install all 4 page tables into the page directory */
    uint32_t dir_flags = PAGE_PRESENT | PAGE_RW | PAGE_USER;
    page_directory[0] = (uint32_t)page_table_0 | dir_flags;
    page_directory[1] = (uint32_t)page_table_1 | dir_flags;
    page_directory[2] = (uint32_t)page_table_2 | dir_flags;
    page_directory[3] = (uint32_t)page_table_3 | dir_flags;

    /* 4. Load CR3 */
    asm volatile("mov %0, %%cr3" : : "r"((uint32_t)page_directory) : "memory");

    /* 5. Enable paging — set bit 31 of CR0 */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}


void paging_fault_handler(registers_t* regs) {
    /* Read CR2 — the faulting linear address */
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    tty_puts("\n  [!] PAGE FAULT  addr=");
    tty_put_hex(fault_addr);
    tty_puts("  err=");
    tty_put_hex(regs->err_code);
    tty_puts("\n      P="); tty_put_uint(regs->err_code & 1);
    tty_puts(" W=");        tty_put_uint((regs->err_code >> 1) & 1);
    tty_puts(" U=");        tty_put_uint((regs->err_code >> 2) & 1);
    tty_puts("\n  Halting.\n");

    asm volatile("cli; hlt");
    while (1);
}
