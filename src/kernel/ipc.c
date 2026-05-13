#include "ipc.h"
#include "scheduler.h"
#include "serial.h"
#include "ktf.h"

extern thread_t threads[];
ipc_mailbox_t mailboxes[MAX_THREADS];

void ipc_init_mailbox(ipc_mailbox_t* mailbox) {
    mailbox->head = 0; mailbox->tail = 0; mailbox->count = 0;
}

void ipc_init(void) {
    for (int i=0; i<MAX_THREADS; i++) ipc_init_mailbox(&mailboxes[i]);
}

int ipc_send(uint32_t receiver_id, ipc_message_t* msg) {
    int receiver_idx = -1;
    for (int i=0; i<MAX_THREADS; i++) {
        if (threads[i].state != THREAD_FREE && threads[i].id == receiver_id) { receiver_idx = i; break; }
    }
    if (receiver_idx == -1) return -1;

    ipc_mailbox_t* mb = &mailboxes[receiver_idx];
    asm volatile("cli");
    if (mb->count >= IPC_MAX_MESSAGES) { asm volatile("sti"); return -1; }

    mb->messages[mb->tail] = *msg;
    mb->tail = (mb->tail + 1) % IPC_MAX_MESSAGES;
    mb->count++;

    ktf_log_event(KTF_EVENT_IPC_SEND, msg->sender_id, receiver_id, msg->message_type);

    asm volatile("sti");
    return 0;
}

int ipc_receive(uint32_t receiver_id, ipc_message_t* msg) {
    int receiver_idx = -1;
    for (int i=0; i<MAX_THREADS; i++) {
        if (threads[i].state != THREAD_FREE && threads[i].id == receiver_id) { receiver_idx = i; break; }
    }
    if (receiver_idx == -1) return -1;

    ipc_mailbox_t* mb = &mailboxes[receiver_idx];
    asm volatile("cli");
    if (mb->count == 0) { asm volatile("sti"); return -1; }

    *msg = mb->messages[mb->head];
    mb->head = (mb->head + 1) % IPC_MAX_MESSAGES;
    mb->count--;

    ktf_log_event(KTF_EVENT_IPC_RECV, receiver_id, msg->sender_id, msg->message_type);

    asm volatile("sti");
    return 0;
}
