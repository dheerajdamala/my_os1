#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "idt.h"

#define PAGE_PRESENT  (1 << 0)
#define PAGE_RW       (1 << 1)
#define PAGE_USER     (1 << 2)

void paging_init(void);
void paging_fault_handler(registers_t* regs);
void page_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

#endif /* PAGING_H */
