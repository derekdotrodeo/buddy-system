/*
 * bbp_mod.c — module-side policy.
 *
 * Implements the card half of BS-SPEC-200: address from the SLOT pin, address
 * filtering, the strict "transmit only after a poll" rule (one packet per poll),
 * the protocol-before-events outgoing queue, identify -> bbp.info, and bbp.error
 * for unrecognized addressed schemas.
 */
#include "bbp_policy.h"
#include <string.h>

/* ---- fixed-size message rings ---- */
static bbp_msg_t *ring_push(bbp_msg_t *buf, uint8_t *head, uint8_t *count, uint8_t depth)
{
    bbp_msg_t *slot;
    if (*count >= depth) {
        return NULL;
    }
    slot = &buf[*head];
    *head = (uint8_t)((*head + 1u) % depth);
    (*count)++;
    return slot;
}

static bbp_msg_t *ring_pop(bbp_msg_t *buf, uint8_t *tail, uint8_t *count, uint8_t depth)
{
    bbp_msg_t *slot;
    if (*count == 0u) {
        return NULL;
    }
    slot = &buf[*tail];
    *tail = (uint8_t)((*tail + 1u) % depth);
    (*count)--;
    return slot;
}

static void fill_msg(bbp_msg_t *m, const char *schema, uint8_t slen,
                     const uint8_t *payload, uint8_t plen)
{
    m->schema_len = slen;
    if (slen > 0u) { memcpy(m->schema, schema, slen); }
    m->payload_len = plen;
    if (plen > 0u) { memcpy(m->payload, payload, plen); }
}

static void enqueue_proto(bbp_mod_t *m, const char *schema, uint8_t slen,
                          const uint8_t *payload, uint8_t plen)
{
    bbp_msg_t *slot = ring_push(m->proto, &m->proto_head, &m->proto_count,
                                BBP_MOD_PROTO_DEPTH);
    if (slot != NULL) {            /* proto queue is small; on overflow, drop */
        fill_msg(slot, schema, slen, payload, plen);
    }
}

static void enqueue_info(bbp_mod_t *m)
{
    uint8_t buf[BBP_INFO_MAX];
    uint8_t n = bbp_info_encode(buf, m->id);
    enqueue_proto(m, BBP_SCHEMA_INFO, (uint8_t)strlen(BBP_SCHEMA_INFO), buf, n);
}

static void enqueue_error(bbp_mod_t *m, uint8_t klass,
                          const char *ctx, uint8_t ctx_len)
{
    uint8_t buf[1u + BBP_MAX_SCHEMA];
    uint8_t n = 0;

    buf[n++] = klass;
    if (ctx_len > BBP_MAX_SCHEMA) {
        ctx_len = BBP_MAX_SCHEMA;
    }
    if (ctx_len > 0u) { memcpy(&buf[n], ctx, ctx_len); n = (uint8_t)(n + ctx_len); }
    enqueue_proto(m, BBP_SCHEMA_ERROR, (uint8_t)strlen(BBP_SCHEMA_ERROR), buf, n);
    m->errors_queued++;
}

/* Release exactly one queued packet to the host (proto first, then events). */
static void release_one(bbp_mod_t *m)
{
    bbp_msg_t *msg = ring_pop(m->proto, &m->proto_tail, &m->proto_count,
                              BBP_MOD_PROTO_DEPTH);
    if (msg == NULL) {
        msg = ring_pop(m->event, &m->event_tail, &m->event_count,
                       BBP_MOD_EVENT_DEPTH);
    }
    m->polls_served++;
    if (msg == NULL) {
        return;                    /* nothing queued: stay silent */
    }
    if (bbp_encode(m->core, BBP_HOST_ADDR, msg->schema, msg->schema_len,
                   msg->payload, msg->payload_len) == BBP_OK) {
        bbp_transmit(m->core);
    }
}

static void handle(bbp_mod_t *m, const bbp_packet_t *pkt)
{
    if (pkt->dest == m->slot) {                    /* addressed to this card */
        if (bbp_schema_eq(pkt, BBP_SCHEMA_POLL)) {
            release_one(m);
            return;
        }
        if (bbp_schema_eq(pkt, BBP_SCHEMA_IDENTIFY)) {
            enqueue_info(m);
            return;
        }
        if (m->on_packet != NULL && m->on_packet(m, pkt, m->app)) {
            return;
        }
        enqueue_error(m, BBP_ERRC_UNKNOWN_SCHEMA, pkt->schema, pkt->schema_len);
    } else if (pkt->dest == BBP_BROADCAST) {       /* transport/sync: no reply */
        if (m->on_packet != NULL) {
            (void)m->on_packet(m, pkt, m->app);
        }
    }
    /* else: addressed to another slot — ignore (§6.2) */
}

bbp_err_t bbp_mod_init(bbp_mod_t *m, bbp_t *core, const bbp_identity_t *id,
                       bbp_mod_handler_fn on_packet, void *app)
{
    uint8_t slot;

    if (m == NULL || core == NULL || id == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (id->vendor == NULL || id->product == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (strlen(id->vendor) > BBP_NAME_MAX || strlen(id->product) > BBP_NAME_MAX) {
        return BBP_ERR_PAYLOAD_TOO_LONG;
    }
    if (core->plat == NULL || core->plat->read_slot == NULL) {
        return BBP_ERR_PLATFORM;
    }
    slot = core->plat->read_slot(core->plat->ctx);
    if (slot < BBP_SLOT_MIN || slot > BBP_SLOT_MAX) {
        return BBP_ERR_INVALID_ADDR;
    }

    memset(m, 0, sizeof *m);
    m->core      = core;
    m->id        = id;
    m->on_packet = on_packet;
    m->app       = app;
    m->slot      = slot;
    bbp_set_self_addr(core, slot);
    return BBP_OK;
}

bbp_err_t bbp_mod_task(bbp_mod_t *m)
{
    bbp_packet_t pkt;
    int b;

    if (m == NULL || m->core == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (m->core->plat == NULL || m->core->plat->rx == NULL) {
        return BBP_ERR_PLATFORM;
    }
    while ((b = m->core->plat->rx(m->core->plat->ctx)) >= 0) {
        bbp_receive_byte(m->core, (uint8_t)b);
        while (bbp_poll(m->core, &pkt) == BBP_OK) {    /* one frame per byte */
            handle(m, &pkt);
        }
    }
    return BBP_OK;
}

bbp_err_t bbp_mod_post_event(bbp_mod_t *m, const char *schema, uint8_t schema_len,
                             const uint8_t *payload, uint8_t payload_len)
{
    bbp_msg_t *slot;

    if (m == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if ((schema_len > 0u && schema == NULL) || (payload_len > 0u && payload == NULL)) {
        return BBP_ERR_NULL_ARG;
    }
    if (schema_len > BBP_MAX_SCHEMA) {
        return BBP_ERR_SCHEMA_TOO_LONG;
    }
    if (payload_len > BBP_MOD_MSG_CAP) {
        return BBP_ERR_PAYLOAD_TOO_LONG;
    }
    slot = ring_push(m->event, &m->event_head, &m->event_count, BBP_MOD_EVENT_DEPTH);
    if (slot == NULL) {
        m->events_dropped++;
        return BBP_ERR_QUEUE_FULL;
    }
    fill_msg(slot, schema, schema_len, payload, payload_len);
    return BBP_OK;
}
