#include "scheduler.h"
#include "memory.h"
#include "serial.h"
#include "ktf.h"

thread_t threads[MAX_THREADS];
thread_t* current_thread = 0;
uint32_t next_thread_id = 1;

extern void switch_to_thread(thread_t* prev, thread_t* next);

void scheduler_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) threads[i].state = THREAD_FREE;
}

thread_t* thread_create(void (*entry_point)(void)) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_FREE) {
            threads[i].id = next_thread_id++;
            threads[i].state = THREAD_READY;
            threads[i].entry_point = entry_point;
            threads[i].stack_base = (uint32_t)pmm_alloc_page();
            uint32_t* stack = (uint32_t*)(threads[i].stack_base + PAGE_SIZE);
            *(--stack) = (uint32_t)entry_point;
            *(--stack) = 0; *(--stack) = 0; *(--stack) = 0; *(--stack) = 0;
            threads[i].esp = (uint32_t)stack;

            ktf_log_event(KTF_EVENT_THREAD_CREATE, threads[i].id, threads[i].stack_base, 0);
            return &threads[i];
        }
    }
    return 0;
}

void schedule(void) {
    if (current_thread == 0) {
        for (int i = 0; i < MAX_THREADS; i++) {
            if (threads[i].state == THREAD_READY) {
                current_thread = &threads[i];
                current_thread->state = THREAD_RUNNING;
                ktf_log_event(KTF_EVENT_SCHEDULER_SWITCH, current_thread->id, 0, 0);
                asm volatile("mov %0, %%esp \n pop %%ebx \n pop %%esi \n pop %%edi \n pop %%ebp \n ret \n" : : "r"(current_thread->esp));
            }
        }
    } else {
        int current_idx = -1;
        for (int i = 0; i < MAX_THREADS; i++) if (&threads[i] == current_thread) { current_idx = i; break; }
        thread_t* next_thread = 0;
        for (int i = 1; i <= MAX_THREADS; i++) {
            int idx = (current_idx + i) % MAX_THREADS;
            if (threads[idx].state == THREAD_READY || threads[idx].state == THREAD_RUNNING) {
                next_thread = &threads[idx];
                break;
            }
        }
        if (next_thread && next_thread != current_thread) {
            thread_t* prev = current_thread;
            prev->state = THREAD_READY;
            current_thread = next_thread;
            current_thread->state = THREAD_RUNNING;

            ktf_log_event(KTF_EVENT_SCHEDULER_SWITCH, current_thread->id, prev->id, 0);
            switch_to_thread(prev, current_thread);
        }
    }
}

void scheduler_yield(void) { schedule(); }
thread_t* get_current_thread(void) { return current_thread; }
