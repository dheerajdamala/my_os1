#include "paging.h"
#include "tty.h"
#include "memory.h"
#include "serial.h"

#define PAGE_SIZE_4K  4096
#define PAGES_PER_TAB 1024

/* Statically allocated, 4 KB aligned structures. */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table_0[1024]   __attribute__((aligned(4096))); /* 0  -  4 MB */
static uint32_t page_table_1[1024]   __attribute__((aligned(4096))); /* 4  -  8 MB */
static uint32_t page_table_2[1024]   __attribute__((aligned(4096))); /* 8  - 12 MB */
static uint32_t page_table_3[1024]   __attribute__((aligned(4096))); /* 12 - 16 MB */

extern uint32_t user_brk_start;
extern uint32_t user_brk_current;

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

void page_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_idx = virtual_addr >> 22;
    uint32_t pt_idx = (virtual_addr >> 12) & 0x3FF;

    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        void* new_pt_phys = pmm_alloc_page();
        if (!new_pt_phys) {
            serial_printf("[PAGING] Out of memory allocating page table!\n");
            return;
        }

        uint32_t* new_pt = (uint32_t*)new_pt_phys;
        for (int i = 0; i < 1024; i++) {
            new_pt[i] = 0;
        }

        page_directory[pd_idx] = (uint32_t)new_pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    uint32_t* page_table = (uint32_t*)(page_directory[pd_idx] & ~0xFFF);
    page_table[pt_idx] = (physical_addr & ~0xFFF) | flags;

    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

void paging_fault_handler(registers_t* regs) {
    /* Read CR2 — the faulting linear address */
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    /* Check if it falls within the valid user-space heap range */
    if (user_brk_start != 0 && fault_addr >= user_brk_start && fault_addr < user_brk_current) {
        void* page_phys = pmm_alloc_page();
        if (!page_phys) {
            serial_printf("[PAGING] Out of physical memory for demand mapping!\n");
            asm volatile("cli; hlt");
        }
        
        page_map(fault_addr, (uint32_t)page_phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
        serial_printf("[PAGING] Demand mapped virtual page 0x%x to physical page 0x%x\n", 
                      fault_addr & ~0xFFF, (uint32_t)page_phys);
        return; // Resume instruction execution
    }

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
