void _start() {
    const char* msg = "Hello from the LOADED ELF binary!\n";
    // sys_print (eax=1, ebx=msg)
    __asm__ volatile("mov $1, %%eax\n"
                     "mov %0, %%ebx\n"
                     "int $0x80" : : "r"(msg) : "eax", "ebx");
    
    // sys_yield (eax=2)
    while(1) {
        __asm__ volatile("mov $2, %%eax\n"
                         "int $0x80" : : : "eax");
        for (volatile int i = 0; i < 5000000; i++);
    }
}
