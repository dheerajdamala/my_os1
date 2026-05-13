#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

void kernel_main(uint32_t magic, uint32_t multiboot_info);

#endif // KERNEL_H
