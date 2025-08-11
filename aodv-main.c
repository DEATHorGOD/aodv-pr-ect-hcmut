#include "contiki.h"
#include "net/rime.h"
#include "dev/serial-line.h"
#include "aodv.h"
#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct broadcast_conn broadcast;
static struct unicast_conn unicast;
static struct runicast_conn runicast;

/* =================================================================== */
/* ======================== RIME CALLBACKS =========================== */
/* =================================================================== */

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
    if (*(uint8_t *)packetbuf_dataptr() == RREQ) {
        AodvRreq *rreq = (AodvRreq *)packetbuf_dataptr();
        if(aodv_is_rreq_seen(rreq)) return;
        
        aodv_update_route(rreq->source_address, from->u8[0], rreq->hop_count + 1, rreq->source_sequence_number, true);
        
        if(rreq->destination_address == rimeaddr_node_addr.u8[0]) {
            aodv_reply_to_rreq(&unicast, from, rreq);
        } else if (aodv_has_fresh_route(rreq)) {
            // simplified reply for educational purposes
            aodv_forward_rrep(&unicast, (AodvRrep*)rreq); 
        } else {
            aodv_forward_rreq(c, rreq);
        }
    }
}

static void
unicast_recv(struct unicast_conn *c, const rimeaddr_t *from)
{
    uint8_t *packet_type = (uint8_t *)packetbuf_dataptr();
    
    if (*packet_type == RREP) {
        AodvRrep *rrep = (AodvRrep *)packetbuf_dataptr();
        aodv_update_route(rrep->destination_address, from->u8[0], rrep->hop_count + 1, rrep->destination_sequence_number, true);
        if(rrep->source_address != rimeaddr_node_addr.u8[0]) {
            aodv_forward_rrep(c, rrep);
        }
    } else if (*packet_type == RERR) {
        AodvRerr *rerr = (AodvRerr *)packetbuf_dataptr();
        // Simplified RERR handling for this example
    }
}

static void
runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
    if (*(uint8_t *)packetbuf_dataptr() == PING) {
        DataPing *ping = (DataPing *)packetbuf_dataptr();
        if(ping->destination_address == rimeaddr_node_addr.u8[0]) {
            printf("Node %d: Received PING from %d!\n", rimeaddr_node_addr.u8[0], ping->source_address);
        } else {
            data_forward(c, ping);
        }
    }
}

static void
runicast_timedout(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
    printf("Node %d: Timeout sending DATA to %d\n", rimeaddr_node_addr.u8[0], to->u8[0]);
    aodv_initiate_rerr(c, to->u8[0]);
}

static void
runicast_sent(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions) {}

static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};
static const struct unicast_callbacks unicast_cb = {unicast_recv};
static const struct runicast_callbacks runicast_cb = {runicast_recv, runicast_sent, runicast_timedout};

/* =================================================================== */
/* ======================== MAIN PROCESS ============================= */
/* =================================================================== */

PROCESS(aodv_main_process, "AODV Main");
AUTOSTART_PROCESSES(&aodv_main_process);

PROCESS_THREAD(aodv_main_process, ev, data)
{
    PROCESS_EXITHANDLER(broadcast_close(&broadcast); unicast_close(&unicast); runicast_close(&runicast);)
    PROCESS_BEGIN();

    aodv_init();
    serial_line_init();

    broadcast_open(&broadcast, 129, &broadcast_cb);
    unicast_open(&unicast, 146, &unicast_cb);
    runicast_open(&runicast, 144, &runicast_cb);
    
    printf("Node %d ready. Commands: ping <id>, pt, rreq <id>, rerr <id>\n", rimeaddr_node_addr.u8[0]);

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
        
        char *line = (char*)data;
        char *cmd = strtok(line, " ");
        char *arg_str = strtok(NULL, " ");
        
        if(cmd != NULL) {
            uint8_t dest = 0;
            if(arg_str != NULL) {
                dest = atoi(arg_str);
            }

            if(!strcmp(cmd, "ping") && dest > 0) {
                data_send(&runicast, dest);
            } else if(!strcmp(cmd, "pt")) {
                aodv_print_rt();
            } else if(!strcmp(cmd, "rreq") && dest > 0) {
                aodv_send_rreq(&broadcast, dest);
            } else if(!strcmp(cmd, "rerr") && dest > 0) {
                aodv_initiate_rerr(&unicast, dest);
            } else {
                printf("Unknown or invalid command.\n");
            }
        }
    }
    PROCESS_END();
}
