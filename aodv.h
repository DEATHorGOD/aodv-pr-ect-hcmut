#ifndef AODV_H
#define AODV_H

#include "net/rime.h"
#include <stdbool.h>

/* --- CONSTANTS --- */
#define RREQ 1
#define RREP 2
#define RERR 3
#define AODV_RT_SIZE 16
#define AODV_SEEN_RREQ_SIZE 16
#define AODV_RREQ_TTL 16


/* --- STRUCTS --- */
typedef struct { uint8_t type, ttl, source_address, destination_address, source_sequence_number, destination_sequence_number, broadcast_id, hop_count; } AodvRreq;
typedef struct { uint8_t type, source_address, destination_address, destination_sequence_number, hop_count; } AodvRrep;
typedef struct { uint8_t type, destination_address, destination_sequence_number; } AodvRerr;
typedef struct AodvRoutingTableEntry { struct AodvRoutingTableEntry *next; uint8_t destination_address, next_hop, hop_count, sequence_number; bool is_destination_valid; } AodvRoutingTableEntry;
typedef struct AodvSeenRreq { struct AodvSeenRreq *next; uint8_t source_address, broadcast_id; } AodvSeenRreq;

/* --- FUNCTION PROTOTYPES --- */
void aodv_init(void);
void aodv_print_rt(void);
uint8_t aodv_lookup_route(uint8_t dest);
void aodv_update_route(uint8_t dest, uint8_t next_hop, uint8_t hops, uint8_t seqno, bool is_valid);
void aodv_invalidate_route(uint8_t dest);
bool aodv_has_fresh_route(AodvRreq *rreq);
bool aodv_is_rreq_seen(AodvRreq *rreq);
void aodv_send_rreq(struct broadcast_conn *bc, uint8_t dest_addr);
void aodv_forward_rreq(struct broadcast_conn *bc, AodvRreq *rreq);
void aodv_send_rrep(struct unicast_conn *uc, rimeaddr_t *next_hop, AodvRrep *rrep);
void aodv_reply_to_rreq(struct unicast_conn *uc, const rimeaddr_t *from, AodvRreq *rreq);
void aodv_forward_rrep(struct unicast_conn *uc, AodvRrep *rrep);
void aodv_initiate_rerr(struct unicast_conn *uc, uint8_t broken_dest);
void aodv_forward_rerr(struct unicast_conn *uc, AodvRerr *rerr, const rimeaddr_t *from);

#endif /* AODV_H */
