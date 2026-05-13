#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "idt.h"

void paging_init(void);
void paging_fault_handler(registers_t* regs);

#endif /* PAGING_H */
