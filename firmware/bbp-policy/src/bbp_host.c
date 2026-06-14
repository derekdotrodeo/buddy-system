/*
 * bbp_host.c — host-side policy.
 *
 * Implements the bus-master half of BS-SPEC-200: the host-controlled polling
 * model, a non-blocking discovery sweep (identify -> poll -> bbp.info per slot,
 * §9.2) with a turnaround timeout (§4.3), and dispatch of card traffic.
 *
 * Discovery is a state machine advanced by bbp_host_task so it never blocks the
 * main loop (and so it interleaves correctly with cards on a single-threaded
 * simulator).
 */
#include "bbp_policy.h"
#include <string.h>

enum { HOST_IDLE = 0, HOST_PROBE, HOST_WAIT };

static bbp_err_t send_frame(bbp_t *core, uint8_t dest, const char *schema,
                            uint8_t slen, const uint8_t *payload, uint8_t plen)
{
    bbp_err_t rc = bbp_encode(core, dest, schema, slen, payload, plen);
    if (rc != BBP_OK) {
        return rc;
    }
    return bbp_transmit(core);
}

static uint32_t now_ms(const bbp_host_t *h)
{
    return h->core->plat->millis(h->core->plat->ctx);
}

bbp_err_t bbp_host_init(bbp_host_t *h, bbp_t *core,
                        bbp_host_handler_fn on_packet, void *app)
{
    if (h == NULL || core == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (core->plat == NULL || core->plat->millis == NULL || core->plat->rx == NULL) {
        return BBP_ERR_PLATFORM;
    }
    memset(h, 0, sizeof *h);
    h->core            = core;
    h->on_packet       = on_packet;
    h->app             = app;
    h->poll_timeout_ms = 5u;        /* default turnaround window (BS-SPEC-200 §4.3) */
    h->disc_state      = HOST_IDLE;
    bbp_set_self_addr(core, BBP_HOST_ADDR);
    return BBP_OK;
}

void bbp_host_set_timeout(bbp_host_t *h, uint32_t ms)
{
    if (h != NULL) {
        h->poll_timeout_ms = ms;
    }
}

bbp_err_t bbp_host_start_discovery(bbp_host_t *h)
{
    uint8_t s;
    if (h == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    for (s = 0; s <= BBP_SLOT_MAX; s++) {
        h->slots[s].present = false;
    }
    h->discovered = 0;
    h->disc_slot  = BBP_SLOT_MIN;
    h->disc_state = HOST_PROBE;
    return BBP_OK;
}

bool bbp_host_discovery_busy(const bbp_host_t *h)
{
    return (h != NULL) && (h->disc_state != HOST_IDLE);
}

/* Move discovery to the next slot, or finish. */
static void disc_advance(bbp_host_t *h)
{
    h->disc_slot++;
    h->disc_state = (h->disc_slot > BBP_SLOT_MAX) ? HOST_IDLE : HOST_PROBE;
}

static void host_handle(bbp_host_t *h, const bbp_packet_t *pkt)
{
    if (h->disc_state == HOST_WAIT &&
        pkt->src == h->disc_slot &&
        bbp_schema_eq(pkt, BBP_SCHEMA_INFO)) {
        bbp_slot_rec_t *rec = &h->slots[h->disc_slot];
        if (bbp_info_decode(pkt->payload, pkt->payload_len, &rec->info)) {
            rec->present = true;
            h->discovered++;
        }
        disc_advance(h);
        return;
    }
    if (h->on_packet != NULL) {
        h->on_packet(h, pkt, h->app);
    }
}

static void disc_step(bbp_host_t *h)
{
    switch (h->disc_state) {
    case HOST_PROBE:
        if (h->disc_slot > BBP_SLOT_MAX) {
            h->disc_state = HOST_IDLE;
            break;
        }
        /* Prepare the response (identify), then solicit it (poll). §9.2 */
        send_frame(h->core, h->disc_slot, BBP_SCHEMA_IDENTIFY,
                   (uint8_t)strlen(BBP_SCHEMA_IDENTIFY), NULL, 0);
        send_frame(h->core, h->disc_slot, BBP_SCHEMA_POLL,
                   (uint8_t)strlen(BBP_SCHEMA_POLL), NULL, 0);
        h->disc_deadline = now_ms(h) + h->poll_timeout_ms;
        h->disc_state    = HOST_WAIT;
        break;

    case HOST_WAIT:
        if ((int32_t)(now_ms(h) - h->disc_deadline) >= 0) {
            h->slots[h->disc_slot].present = false;   /* silence -> empty slot */
            disc_advance(h);
        }
        break;

    default:
        break;
    }
}

bbp_err_t bbp_host_task(bbp_host_t *h)
{
    bbp_packet_t pkt;
    int b;

    if (h == NULL || h->core == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (h->core->plat == NULL || h->core->plat->rx == NULL) {
        return BBP_ERR_PLATFORM;
    }
    while ((b = h->core->plat->rx(h->core->plat->ctx)) >= 0) {
        bbp_receive_byte(h->core, (uint8_t)b);
        while (bbp_poll(h->core, &pkt) == BBP_OK) {
            host_handle(h, &pkt);
        }
    }
    disc_step(h);
    return BBP_OK;
}

bbp_err_t bbp_host_poll_slot(bbp_host_t *h, uint8_t slot)
{
    if (h == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (slot < BBP_SLOT_MIN || slot > BBP_SLOT_MAX) {
        return BBP_ERR_INVALID_ADDR;
    }
    return send_frame(h->core, slot, BBP_SCHEMA_POLL,
                      (uint8_t)strlen(BBP_SCHEMA_POLL), NULL, 0);
}

bbp_err_t bbp_host_send(bbp_host_t *h, uint8_t dest,
                        const char *schema, uint8_t schema_len,
                        const uint8_t *payload, uint8_t payload_len)
{
    if (h == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (dest < BBP_SLOT_MIN || dest > BBP_SLOT_MAX) {
        return BBP_ERR_INVALID_ADDR;
    }
    return send_frame(h->core, dest, schema, schema_len, payload, payload_len);
}

bbp_err_t bbp_host_broadcast(bbp_host_t *h,
                             const char *schema, uint8_t schema_len,
                             const uint8_t *payload, uint8_t payload_len)
{
    if (h == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    return send_frame(h->core, BBP_BROADCAST, schema, schema_len, payload, payload_len);
}

const bbp_slot_rec_t *bbp_host_slot(const bbp_host_t *h, uint8_t slot)
{
    if (h == NULL || slot < BBP_SLOT_MIN || slot > BBP_SLOT_MAX) {
        return NULL;
    }
    return &h->slots[slot];
}
