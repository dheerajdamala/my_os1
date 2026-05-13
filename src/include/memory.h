#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include <stddef.h>
#define PAGE_SIZE 4096
void pmm_init(uint32_t mem_size);
void* pmm_alloc_page(void);
void pmm_free_page(void* p);
void kheap_init(void);
void* kmalloc(size_t size);
void kfree(void* p);
#endif
