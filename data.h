#ifndef DATA_H
#define DATA_H

#include "net/rime.h"

#define PING 4
#define DATA_MAX_RETRANSMISSIONS 4

typedef struct { uint8_t type, seqno, source_address, destination_address; } DataPing;

void data_send(struct runicast_conn *rc, uint8_t dest);
void data_forward(struct runicast_conn *rc, DataPing *ping);

#endif /* DATA_H */
