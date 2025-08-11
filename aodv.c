#include "aodv.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include <stdio.h>

MEMB(rt_memb, AodvRoutingTableEntry, AODV_RT_SIZE);
LIST(routing_table);
MEMB(seen_memb, AodvSeenRreq, AODV_SEEN_RREQ_SIZE);
LIST(seen_rreq_list);

static uint8_t my_seqno = 0;
static uint8_t my_bcast_id = 0;

void aodv_init(void) { list_init(routing_table); memb_init(&rt_memb); list_init(seen_rreq_list); memb_init(&seen_memb); }
void aodv_print_rt(void) {
    AodvRoutingTableEntry *e;
    printf("--- AODV RT (Node %d) ---\n", rimeaddr_node_addr.u8[0]);
    for(e = list_head(routing_table); e; e = e->next) printf("| D:%-3d| NH:%-3d| H:%-2d| S#:%-3d| V:%s\n", e->destination_address, e->next_hop, e->hop_count, e->sequence_number, e->is_destination_valid ? "Y" : "N");
}
static AodvRoutingTableEntry* find_route(uint8_t dest) {
    AodvRoutingTableEntry *e;
    for(e = list_head(routing_table); e; e = e->next) if(e->destination_address == dest) return e;
    return NULL;
}
uint8_t aodv_lookup_route(uint8_t dest) { AodvRoutingTableEntry *e = find_route(dest); return (e && e->is_destination_valid) ? e->next_hop : 0; }
void aodv_update_route(uint8_t dest, uint8_t next_hop, uint8_t hops, uint8_t seqno, bool is_valid) {
    AodvRoutingTableEntry *e = find_route(dest);
    if (!e) { e = memb_alloc(&rt_memb); if(!e) return; e->destination_address = dest; list_add(routing_table, e); }
    if (e->sequence_number < seqno || (e->sequence_number == seqno && e->hop_count > hops)) {
        e->next_hop = next_hop; e->hop_count = hops; e->sequence_number = seqno; e->is_destination_valid = is_valid;
    }
}
void aodv_invalidate_route(uint8_t dest) { AodvRoutingTableEntry *e = find_route(dest); if(e) e->is_destination_valid = 0; }
bool aodv_has_fresh_route(AodvRreq *rreq) {
    AodvRoutingTableEntry *e = find_route(rreq->destination_address);
    return (e && e->is_destination_valid && e->sequence_number >= rreq->destination_sequence_number);
}
bool aodv_is_rreq_seen(AodvRreq *rreq) {
    AodvSeenRreq *s;
    for(s = list_head(seen_rreq_list); s; s = s->next) if(s->source_address == rreq->source_address && s->broadcast_id == rreq->broadcast_id) return true;
    s = memb_alloc(&seen_memb); if(s) { s->source_address = rreq->source_address; s->broadcast_id = rreq->broadcast_id; list_add(seen_rreq_list, s); }
    return false;
}
void aodv_send_rreq(struct broadcast_conn *bc, uint8_t dest_addr) {
    AodvRreq rreq;
    rreq.type = RREQ; my_bcast_id++; rreq.broadcast_id = my_bcast_id;
    rreq.source_address = rimeaddr_node_addr.u8[0]; my_seqno++; rreq.source_sequence_number = my_seqno;
    rreq.destination_address = dest_addr; rreq.destination_sequence_number = 0;
    rreq.hop_count = 0; rreq.ttl = AODV_RREQ_TTL;
    packetbuf_copyfrom(&rreq, sizeof(rreq)); broadcast_send(bc);
}
void aodv_forward_rreq(struct broadcast_conn *bc, AodvRreq *rreq) {
    rreq->hop_count++; rreq->ttl--;
    if(rreq->ttl > 0) { packetbuf_copyfrom(rreq, sizeof(AodvRreq)); broadcast_send(bc); }
}
void aodv_send_rrep(struct unicast_conn *uc, rimeaddr_t *next_hop, AodvRrep *rrep) { packetbuf_copyfrom(rrep, sizeof(AodvRrep)); unicast_send(uc, next_hop); }
void aodv_reply_to_rreq(struct unicast_conn *uc, const rimeaddr_t *from, AodvRreq *rreq) {
    AodvRrep rrep; rimeaddr_t next_hop_addr;
    rrep.type = RREP; rrep.source_address = rreq->source_address;
    rrep.destination_address = rimeaddr_node_addr.u8[0]; my_seqno++; rrep.destination_sequence_number = my_seqno;
    rrep.hop_count = 0;
    rimeaddr_copy(&next_hop_addr, from); aodv_send_rrep(uc, &next_hop_addr, &rrep);
}
void aodv_forward_rrep(struct unicast_conn *uc, AodvRrep *rrep) {
    uint8_t next_hop_id = aodv_lookup_route(rrep->source_address);
    if(next_hop_id != 0) { rimeaddr_t next_hop_addr; next_hop_addr.u8[0] = next_hop_id; next_hop_addr.u8[1] = 0; aodv_send_rrep(uc, &next_hop_addr, rrep); }
}
void aodv_initiate_rerr(struct unicast_conn *uc, uint8_t broken_dest) {
    AodvRoutingTableEntry *e = find_route(broken_dest); if(!e) return;
    AodvRerr rerr; rerr.type = RERR; rerr.destination_address = broken_dest; e->sequence_number++; rerr.destination_sequence_number = e->sequence_number;
    aodv_invalidate_route(broken_dest);
    // Logic to broadcast/unicast RERR to precursors is complex, simplified to invalidation for now.
}
void aodv_forward_rerr(struct unicast_conn *uc, AodvRerr *rerr, const rimeaddr_t *from){} // Simplified
