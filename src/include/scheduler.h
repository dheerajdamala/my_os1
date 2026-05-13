#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdint.h>
#define MAX_THREADS 16
#define THREAD_STACK_SIZE 4096
typedef enum { THREAD_FREE, THREAD_READY, THREAD_RUNNING, THREAD_BLOCKED } thread_state_t;
typedef struct {
    uint32_t esp;
    uint32_t id;
    thread_state_t state;
    uint32_t stack_base;
    void (*entry_point)(void);
} thread_t;
void scheduler_init(void);
thread_t* thread_create(void (*entry_point)(void));
void scheduler_yield(void);
void schedule(void);
thread_t* get_current_thread(void);
#endif
