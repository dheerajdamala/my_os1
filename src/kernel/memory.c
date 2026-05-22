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

typedef struct block_header {
    size_t size;
    int is_free;
    struct block_header* next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

static block_header_t* heap_first = 0;
uint32_t heap_start = 0;

void kheap_init(void) {
    void* p = pmm_alloc_page();
    for (int i = 1; i < 10; i++) {
        pmm_alloc_page();
    }
    
    heap_start = (uint32_t)p;
    heap_first = (block_header_t*)p;
    heap_first->size = (10 * PAGE_SIZE) - HEADER_SIZE;
    heap_first->is_free = 1;
    heap_first->next = 0;
}

void* kmalloc(size_t size) {
    if (size == 0) return 0;
    
    // Align to 4 bytes
    if (size % 4 != 0) {
        size += 4 - (size % 4);
    }
    
    block_header_t* curr = heap_first;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            // Split block if large enough
            if (curr->size >= size + HEADER_SIZE + 4) {
                block_header_t* new_block = (block_header_t*)((uint32_t)curr + HEADER_SIZE + size);
                new_block->size = curr->size - size - HEADER_SIZE;
                new_block->is_free = 1;
                new_block->next = curr->next;
                
                curr->size = size;
                curr->next = new_block;
            }
            
            curr->is_free = 0;
            return (void*)((uint32_t)curr + HEADER_SIZE);
        }
        curr = curr->next;
    }
    
    // Grow heap by 1 page
    void* new_page = pmm_alloc_page();
    if (!new_page) {
        serial_printf("[KERNEL HEAP] Out of memory!\n");
        return 0;
    }
    
    block_header_t* last = heap_first;
    while (last && last->next) {
        last = last->next;
    }
    
    uint32_t last_end = (uint32_t)last + HEADER_SIZE + last->size;
    if (last && last->is_free && last_end == (uint32_t)new_page) {
        last->size += PAGE_SIZE;
        return kmalloc(size);
    } else {
        block_header_t* new_block = (block_header_t*)new_page;
        new_block->size = PAGE_SIZE - HEADER_SIZE;
        new_block->is_free = 1;
        new_block->next = 0;
        
        if (last) {
            last->next = new_block;
        } else {
            heap_first = new_block;
        }
        return kmalloc(size);
    }
}

void kfree(void* p) {
    if (!p) return;
    
    block_header_t* header = (block_header_t*)((uint32_t)p - HEADER_SIZE);
    header->is_free = 1;
    
    // Coalesce adjacent free blocks
    block_header_t* curr = heap_first;
    while (curr) {
        if (curr->is_free && curr->next && curr->next->is_free) {
            uint32_t curr_end = (uint32_t)curr + HEADER_SIZE + curr->size;
            if (curr_end == (uint32_t)curr->next) {
                curr->size += HEADER_SIZE + curr->next->size;
                curr->next = curr->next->next;
                continue;
            }
        }
        curr = curr->next;
    }
}

uint32_t kheap_used(void) {
    uint32_t used = 0;
    block_header_t* curr = heap_first;
    while (curr) {
        if (!curr->is_free) {
            used += curr->size + HEADER_SIZE;
        }
        curr = curr->next;
    }
    return used;
}
