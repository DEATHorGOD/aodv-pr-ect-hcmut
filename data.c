#include "data.h"
#include "aodv.h"

// --- THÊM HÀM PRINT CHI TIẾT ---
void data_print_ping(const char* action, DataPing *ping) {
    printf("%s DATA-PING: Source: %u | Destination: %u\n",
           action, ping->source_address, ping->destination_address);
}

// Biến extern này phải được khai báo trong một file .c (ở đây hoặc aodv.c)
extern struct broadcast_conn broadcast;

void data_send(struct runicast_conn *rc, uint8_t dest) {
    static DataPing ping; static uint8_t my_ping_seqno = 0;
    ping.type = PING; my_ping_seqno++; ping.seqno = my_ping_seqno;
    ping.source_address = rimeaddr_node_addr.u8[0];
    ping.destination_address = dest;
    data_forward(rc, &ping);
}
void data_forward(struct runicast_conn *rc, DataPing *ping) {
    uint8_t next_hop_id = aodv_lookup_route(ping->destination_address);
    if(next_hop_id == 0) {
        printf("Node %d: No route to %d, initiating RREQ.\n", rimeaddr_node_addr.u8[0], ping->destination_address);
        aodv_send_rreq(&broadcast, ping->destination_address);
        return;
    }
    rimeaddr_t next_hop_addr; next_hop_addr.u8[0] = next_hop_id; next_hop_addr.u8[1] = 0;
    packetbuf_copyfrom(ping, sizeof(DataPing));
    runicast_send(rc, &next_hop_addr, DATA_MAX_RETRANSMISSIONS);
}

