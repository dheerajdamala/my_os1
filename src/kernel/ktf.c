#include "ktf.h"
#include "serial.h"
#include "timer.h"

void ktf_init(void) {
    serial_printf("[KTF] Kernel Telemetry Framework Initialized\n");
}

static const char* event_type_to_string(ktf_event_type_t type) {
    switch (type) {
        case KTF_EVENT_THREAD_CREATE: return "THREAD_CREATE";
        case KTF_EVENT_THREAD_EXIT: return "THREAD_EXIT";
        case KTF_EVENT_IPC_SEND: return "IPC_SEND";
        case KTF_EVENT_IPC_RECV: return "IPC_RECV";
        case KTF_EVENT_SYSCALL: return "SYSCALL";
        case KTF_EVENT_MEMORY_ALLOC: return "MEMORY_ALLOC";
        case KTF_EVENT_MEMORY_FREE: return "MEMORY_FREE";
        case KTF_EVENT_SCHEDULER_SWITCH: return "SCHEDULER_SWITCH";
        default: return "UNKNOWN";
    }
}

void ktf_log_event(ktf_event_type_t type, uint32_t thread_id, uint32_t data1, uint32_t data2) {
    uint32_t ts = timer_get_ticks();
    serial_printf("{\"ktf_event\": \"%s\", \"timestamp\": %d, \"thread_id\": %d, \"data1\": %d, \"data2\": %d}\n",
                  event_type_to_string(type), ts, thread_id, data1, data2);
}
