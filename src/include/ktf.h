#ifndef KTF_H
#define KTF_H

#include <stdint.h>

typedef enum {
    KTF_EVENT_THREAD_CREATE,
    KTF_EVENT_THREAD_EXIT,
    KTF_EVENT_IPC_SEND,
    KTF_EVENT_IPC_RECV,
    KTF_EVENT_SYSCALL,
    KTF_EVENT_MEMORY_ALLOC,
    KTF_EVENT_MEMORY_FREE,
    KTF_EVENT_SCHEDULER_SWITCH
} ktf_event_type_t;

void ktf_init(void);
void ktf_log_event(ktf_event_type_t type, uint32_t thread_id, uint32_t data1, uint32_t data2);

#endif // KTF_H
