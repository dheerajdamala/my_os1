#ifndef IPC_H
#define IPC_H
#include <stdint.h>
#define IPC_MAX_MESSAGES 16
typedef struct {
    uint32_t sender_id;
    uint32_t message_type;
    uint32_t data1;
    uint32_t data2;
} ipc_message_t;
typedef struct {
    ipc_message_t messages[IPC_MAX_MESSAGES];
    int head; int tail; int count;
} ipc_mailbox_t;
void ipc_init_mailbox(ipc_mailbox_t* mailbox);
void ipc_init(void);
int ipc_send(uint32_t receiver_id, ipc_message_t* msg);
int ipc_receive(uint32_t receiver_id, ipc_message_t* msg);
#endif
