typedef unsigned int uint32_t;

static void print(const char* msg) {
    __asm__ volatile("int $0x80"
                     :
                     : "a"(1), "b"(msg)
                     : "memory");
}

static uint32_t brk(uint32_t addr) {
    uint32_t ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(3), "b"(addr)
                     : "memory");
    return ret;
}

void _start() {
    print("Hello from the LOADED ELF binary!\n");
    print("Testing dynamic heap allocation and demand paging...\n");

    uint32_t current_brk = brk(0);
    if (current_brk == 0) {
        print("Error: Initial break is 0!\n");
    } else {
        print("Initial break fetched successfully. Requesting 8KB expansion...\n");
        uint32_t new_brk = brk(current_brk + 8192);
        if (new_brk == current_brk) {
            print("Error: Heap expansion failed!\n");
        } else {
            print("Heap expanded. Writing to page 1 of new heap (triggers first fault)...\n");
            volatile char* ptr1 = (volatile char*)(current_brk + 100);
            *ptr1 = 'X';
            
            print("Writing to page 2 of new heap (triggers second fault)...\n");
            volatile char* ptr2 = (volatile char*)(current_brk + 4200);
            *ptr2 = 'Y';
            
            print("Reading back values to verify...\n");
            if (*ptr1 == 'X' && *ptr2 == 'Y') {
                print("SUCCESS: Demand paging and user-space heap allocation verified successfully!\n");
            } else {
                print("FAILURE: Value mismatch in heap!\n");
            }
        }
    }

    // sys_yield (eax=2) loop
    while(1) {
        __asm__ volatile("int $0x80" : : "a"(2) : "memory");
        for (volatile int i = 0; i < 5000000; i++);
    }
}
