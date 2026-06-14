/*
 * bbp_policy.h — BBP policy layer (host + module) on top of bbp-core.
 *
 * The core (bbp-core) is pure framing: bytes <-> validated packets. This layer
 * implements the *rules* of BS-SPEC-200: the host-controlled polling model,
 * discovery, reserved-schema handling, the per-card outgoing queue (protocol
 * responses ahead of events), and bbp.error reporting.
 *
 * Both sides are driven by a task function called from the main loop. Each task
 * pulls inbound bytes via the platform's non-blocking rx() and parses
 * incrementally (the core buffers a single packet, so we poll after each byte).
 *
 * Platform-independent C99. No dynamic memory. Caller allocates all structs.
 */
#ifndef BBP_POLICY_H
#define BBP_POLICY_H

#include "bbp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- reserved schema names (BS-SPEC-200 §8) ---- */
#define BBP_SCHEMA_IDENTIFY "bbp.identify"
#define BBP_SCHEMA_INFO     "bbp.info"
#define BBP_SCHEMA_POLL     "bbp.poll"
#define BBP_SCHEMA_ERROR    "bbp.error"
#define BBP_SCHEMA_START    "bbp.start"
#define BBP_SCHEMA_STOP     "bbp.stop"
#define BBP_SCHEMA_CONTINUE "bbp.continue"
#define BBP_SCHEMA_CLOCK    "bbp.clock"

#ifndef BBP_NAME_MAX
#define BBP_NAME_MAX 24u    /* max vendor / product string length */
#endif

/* ---- module identity (reported in bbp.info, BS-SPEC-200 §9.3) ---- */
typedef struct {
    const char *vendor;        /* e.g. "BUDDY"  (<= BBP_NAME_MAX) */
    const char *product;       /* e.g. "POT-6"  (<= BBP_NAME_MAX) */
    uint8_t     version_major;
    uint8_t     version_minor;
    uint8_t     slot_count;    /* slots occupied; >= 1 */
} bbp_identity_t;

/* ---- decoded bbp.info (host side) ---- */
typedef struct {
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t slot_count;
    uint8_t vendor_len;
    char    vendor[BBP_NAME_MAX];   /* NOT NUL-terminated; use vendor_len */
    uint8_t product_len;
    char    product[BBP_NAME_MAX];  /* NOT NUL-terminated; use product_len */
} bbp_info_t;

/* bbp.info payload wire layout (BS-SPEC-200 §9.3):
 *   [0] slot_count  [1] version_major  [2] version_minor
 *   [3] vendor_len  [4..] vendor       [.] product_len  [..] product
 */
#define BBP_INFO_MAX (3u + 1u + BBP_NAME_MAX + 1u + BBP_NAME_MAX)
uint8_t bbp_info_encode(uint8_t *buf, const bbp_identity_t *id);  /* -> bytes written */
bool    bbp_info_decode(const uint8_t *p, uint8_t len, bbp_info_t *out);

/* ====================================================================== */
/*  Module policy                                                          */
/* ====================================================================== */

#ifndef BBP_MOD_PROTO_DEPTH
#define BBP_MOD_PROTO_DEPTH 4u    /* queued protocol responses (info/error) */
#endif
#ifndef BBP_MOD_EVENT_DEPTH
#define BBP_MOD_EVENT_DEPTH 8u    /* queued application events */
#endif
#ifndef BBP_MOD_MSG_CAP
#define BBP_MOD_MSG_CAP 64u       /* max payload bytes per queued message */
#endif

typedef struct {
    uint8_t schema_len;
    char    schema[BBP_MAX_SCHEMA];
    uint8_t payload_len;
    uint8_t payload[BBP_MOD_MSG_CAP];
} bbp_msg_t;

typedef struct bbp_mod bbp_mod_t;

/*
 * Application handler. Invoked for packets the policy does not consume itself:
 * broadcasts (transport/sync) and addressed application schemas. Return true if
 * the schema was recognized; returning false for an *addressed* packet makes the
 * policy queue a bbp.error (UNKNOWN_SCHEMA). The return value is ignored for
 * broadcasts (a card cannot answer a broadcast). The handler may enqueue a
 * reply with bbp_mod_post_event().
 */
typedef bool (*bbp_mod_handler_fn)(bbp_mod_t *m, const bbp_packet_t *pkt, void *app);

struct bbp_mod {
    bbp_t                *core;
    const bbp_identity_t *id;
    bbp_mod_handler_fn    on_packet;
    void                 *app;
    uint8_t               slot;          /* this card's address (1..6) */

    /* outgoing queues: protocol responses are released before events */
    bbp_msg_t proto[BBP_MOD_PROTO_DEPTH];
    uint8_t   proto_head, proto_tail, proto_count;
    bbp_msg_t event[BBP_MOD_EVENT_DEPTH];
    uint8_t   event_head, event_tail, event_count;

    /* diagnostics */
    uint32_t  polls_served;
    uint32_t  errors_queued;
    uint32_t  events_dropped;            /* event queue was full on post */
};

/* Initialize a module. Reads the slot from platform read_slot() and adopts it as
   the BBP address. Requires read_slot to return a valid slot (1..6). */
bbp_err_t bbp_mod_init(bbp_mod_t *m, bbp_t *core, const bbp_identity_t *id,
                       bbp_mod_handler_fn on_packet, void *app);

/* Service the module: drain inbound bytes, handle protocol, dispatch to the app,
   release a queued packet on each poll. Call from the main loop. */
bbp_err_t bbp_mod_task(bbp_mod_t *m);

/* Queue an outgoing event (released when the card is next polled, after any
   pending protocol responses). Returns BBP_ERR_QUEUE_FULL if the event queue
   is full. */
bbp_err_t bbp_mod_post_event(bbp_mod_t *m, const char *schema, uint8_t schema_len,
                             const uint8_t *payload, uint8_t payload_len);

/* ====================================================================== */
/*  Host policy                                                            */
/* ====================================================================== */

typedef struct bbp_host bbp_host_t;

/* Invoked for every packet a card sends during normal operation (events and
   application responses); bbp.info during discovery is consumed internally. */
typedef void (*bbp_host_handler_fn)(bbp_host_t *h, const bbp_packet_t *pkt, void *app);

typedef struct {
    bool       present;
    bbp_info_t info;
} bbp_slot_rec_t;

struct bbp_host {
    bbp_t              *core;
    bbp_host_handler_fn on_packet;
    void               *app;
    uint32_t            poll_timeout_ms;          /* discovery turnaround (§4.3) */
    bbp_slot_rec_t      slots[BBP_SLOT_MAX + 1u];  /* index by slot 1..6 */

    /* discovery state machine */
    uint8_t  disc_state;
    uint8_t  disc_slot;
    uint32_t disc_deadline;
    uint32_t discovered;                          /* present count, last sweep */
};

/* Initialize a host. Requires platform millis() and rx(). Sets address to host. */
bbp_err_t bbp_host_init(bbp_host_t *h, bbp_t *core,
                        bbp_host_handler_fn on_packet, void *app);

/* Set the per-slot discovery turnaround timeout (default 5 ms). */
void bbp_host_set_timeout(bbp_host_t *h, uint32_t ms);

/* Begin a discovery sweep of slots 1..6. Non-blocking; advanced by bbp_host_task. */
bbp_err_t bbp_host_start_discovery(bbp_host_t *h);
bool      bbp_host_discovery_busy(const bbp_host_t *h);

/* Service the host: drain inbound bytes, dispatch packets, advance discovery. */
bbp_err_t bbp_host_task(bbp_host_t *h);

/* Solicit one queued packet from a slot (drains events). Response arrives via
   on_packet on a later bbp_host_task. */
bbp_err_t bbp_host_poll_slot(bbp_host_t *h, uint8_t slot);

/* Send an addressed command to a slot, or a broadcast (transport/sync). */
bbp_err_t bbp_host_send(bbp_host_t *h, uint8_t dest,
                        const char *schema, uint8_t schema_len,
                        const uint8_t *payload, uint8_t payload_len);
bbp_err_t bbp_host_broadcast(bbp_host_t *h,
                             const char *schema, uint8_t schema_len,
                             const uint8_t *payload, uint8_t payload_len);

/* Read a discovered slot record (slot 1..6), or NULL if out of range. */
const bbp_slot_rec_t *bbp_host_slot(const bbp_host_t *h, uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif /* BBP_POLICY_H */
