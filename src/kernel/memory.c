#include "memory.h"
#include "serial.h"

#define MAX_BLOCKS 32768
uint32_t memory_bitmap[MAX_BLOCKS / 32];
uint32_t total_blocks = 0;
uint32_t used_blocks = 0;

static inline void bitmap_set(uint32_t bit) { memory_bitmap[bit / 32] |= (1 << (bit % 32)); }
static inline void bitmap_clear(uint32_t bit) { memory_bitmap[bit / 32] &= ~(1 << (bit % 32)); }
static inline int bitmap_test(uint32_t bit) { return memory_bitmap[bit / 32] & (1 << (bit % 32)); }

void pmm_init(uint32_t mem_size) {
    total_blocks = mem_size / PAGE_SIZE;
    if (total_blocks > MAX_BLOCKS) total_blocks = MAX_BLOCKS;
    for (uint32_t i = 0; i < MAX_BLOCKS / 32; i++) memory_bitmap[i] = 0;
    for (uint32_t i = 0; i < 1024; i++) { bitmap_set(i); used_blocks++; }
}

void* pmm_alloc_page(void) {
    if (used_blocks >= total_blocks) return 0;
    for (uint32_t i = 0; i < total_blocks / 32; i++) {
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                int bit = i * 32 + j;
                if (!bitmap_test(bit)) {
                    bitmap_set(bit);
                    used_blocks++;
                    return (void*)(bit * PAGE_SIZE);
                }
            }
        }
    }
    return 0;
}

void pmm_free_page(void* p) {
    uint32_t bit = (uint32_t)p / PAGE_SIZE;
    if (bitmap_test(bit)) { bitmap_clear(bit); used_blocks--; }
}

uint32_t heap_start;
uint32_t heap_current;

void kheap_init(void) {
    void* p = pmm_alloc_page();
    for (int i=1; i<10; i++) pmm_alloc_page();
    heap_start = (uint32_t)p;
    heap_current = heap_start;
}

void* kmalloc(size_t size) {
    if (size % 4 != 0) size += 4 - (size % 4);
    uint32_t addr = heap_current;
    heap_current += size;
    return (void*)addr;
}

void kfree(void* p) { (void)p; }
