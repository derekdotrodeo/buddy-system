/*
 * test_policy.c — integration test for the BBP policy layer.
 *
 * Wires a host and two modules (slots 2 and 4) onto the desktop virtual bus and
 * exercises discovery, event draining, application commands with replies,
 * unrecognized-schema errors, and broadcast/transport. A cooperative scheduler
 * runs each node's task once per tick and advances the virtual clock, so the
 * non-blocking host and modules interleave deterministically.
 *
 *   make test   (from firmware/bbp-policy)
 */
#include "bbp_policy.h"
#include "bbp_desktop.h"
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) do {                                              \
    if (!(cond)) {                                                    \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        failures++;                                                   \
    }                                                                 \
} while (0)

/* ---- application state ---- */
typedef struct {
    uint8_t value;        /* value reported by "ctrl.get" */
    bool    got_start;    /* saw a broadcast bbp.start */
} mod_app_t;

typedef struct {
    bool    got;
    uint8_t last_src;
    char    schema[BBP_MAX_SCHEMA];
    uint8_t schema_len;
    uint8_t payload[BBP_MAX_PAYLOAD];
    uint8_t payload_len;
    uint8_t last_err_class;
} host_app_t;

static bool host_schema_is(const host_app_t *a, const char *s)
{
    size_t n = strlen(s);
    return n == a->schema_len && memcmp(a->schema, s, n) == 0;
}

/* ---- handlers ---- */
static bool mod_on_packet(bbp_mod_t *m, const bbp_packet_t *pkt, void *app)
{
    mod_app_t *a = (mod_app_t *)app;

    if (pkt->dest == BBP_BROADCAST) {
        if (bbp_schema_eq(pkt, BBP_SCHEMA_START)) {
            a->got_start = true;
        }
        return true;
    }
    if (bbp_schema_eq(pkt, "ctrl.get")) {
        uint8_t v = a->value;
        bbp_mod_post_event(m, "ctrl.value", 10, &v, 1);   /* reply, drained on poll */
        return true;
    }
    return false;   /* unknown addressed schema -> policy queues bbp.error */
}

static void host_on_packet(bbp_host_t *h, const bbp_packet_t *pkt, void *app)
{
    host_app_t *a = (host_app_t *)app;
    (void)h;
    a->got        = true;
    a->last_src   = pkt->src;
    a->schema_len = pkt->schema_len;
    if (pkt->schema_len > 0) { memcpy(a->schema, pkt->schema, pkt->schema_len); }
    a->payload_len = pkt->payload_len;
    if (pkt->payload_len > 0) { memcpy(a->payload, pkt->payload, pkt->payload_len); }
    if (bbp_schema_eq(pkt, BBP_SCHEMA_ERROR) && pkt->payload_len >= 1) {
        a->last_err_class = pkt->payload[0];
    }
}

/* ---- cooperative scheduler ---- */
static bbp_host_t *g_host;
static bbp_mod_t  *g_mods[2];
static bbp_vbus_t *g_bus;

static void tick(void)
{
    bbp_host_task(g_host);
    bbp_mod_task(g_mods[0]);
    bbp_mod_task(g_mods[1]);
    bbp_vbus_advance(g_bus, 1);   /* 1 ms per tick */
}

static void run_ticks(int n)
{
    int i;
    for (i = 0; i < n; i++) { tick(); }
}

int main(void)
{
    bbp_vbus_t  bus;
    bbp_vport_t hp, p2, p4;
    bbp_t       hc, c2, c4;
    bbp_host_t  host;
    bbp_mod_t   mod2, mod4;
    host_app_t  happ;
    mod_app_t   a2, a4;
    const bbp_identity_t id2 = { "BUDDY", "POT-6",    1, 0, 1 };
    const bbp_identity_t id4 = { "BUDDY", "OLED-128", 1, 2, 2 };
    int guard;

    memset(&happ, 0, sizeof happ);
    memset(&a2, 0, sizeof a2);   a2.value = 7;
    memset(&a4, 0, sizeof a4);   a4.value = 9;

    /* bus + nodes: host (no slot), modules in slots 2 and 4 */
    bbp_vbus_init(&bus);
    CHECK(bbp_vbus_attach(&bus, &hp, 0) == BBP_OK);
    CHECK(bbp_vbus_attach(&bus, &p2, 2) == BBP_OK);
    CHECK(bbp_vbus_attach(&bus, &p4, 4) == BBP_OK);
    CHECK(bbp_init(&hc, &hp.plat, BBP_HOST_ADDR) == BBP_OK);
    CHECK(bbp_init(&c2, &p2.plat, 0) == BBP_OK);   /* addr adopted from slot pin */
    CHECK(bbp_init(&c4, &p4.plat, 0) == BBP_OK);

    CHECK(bbp_host_init(&host, &hc, host_on_packet, &happ) == BBP_OK);
    bbp_host_set_timeout(&host, 3);
    CHECK(bbp_mod_init(&mod2, &c2, &id2, mod_on_packet, &a2) == BBP_OK);
    CHECK(bbp_mod_init(&mod4, &c4, &id4, mod_on_packet, &a4) == BBP_OK);
    CHECK(mod2.slot == 2 && mod4.slot == 4);

    g_host = &host; g_mods[0] = &mod2; g_mods[1] = &mod4; g_bus = &bus;

    /* ---- 1. discovery ---- */
    printf("test_discovery\n");
    CHECK(bbp_host_start_discovery(&host) == BBP_OK);
    guard = 0;
    while (bbp_host_discovery_busy(&host) && guard++ < 1000) { tick(); }
    CHECK(guard < 1000);

    {
        const bbp_slot_rec_t *s2 = bbp_host_slot(&host, 2);
        const bbp_slot_rec_t *s4 = bbp_host_slot(&host, 4);
        CHECK(s2 != NULL && s2->present);
        CHECK(s2->info.slot_count == 1);
        CHECK(s2->info.version_major == 1 && s2->info.version_minor == 0);
        CHECK(s2->info.vendor_len == 5  && memcmp(s2->info.vendor,  "BUDDY", 5) == 0);
        CHECK(s2->info.product_len == 5 && memcmp(s2->info.product, "POT-6", 5) == 0);
        CHECK(s4 != NULL && s4->present);
        CHECK(s4->info.slot_count == 2);
        CHECK(s4->info.product_len == 8 && memcmp(s4->info.product, "OLED-128", 8) == 0);
        CHECK(!bbp_host_slot(&host, 1)->present);
        CHECK(!bbp_host_slot(&host, 3)->present);
        CHECK(!bbp_host_slot(&host, 5)->present);   /* OLED-128 secondary: silent */
        CHECK(!bbp_host_slot(&host, 6)->present);
        CHECK(host.discovered == 2);
    }

    /* ---- 2. event draining: module posts, host polls ---- */
    printf("test_event_drain\n");
    {
        uint8_t v = 42;
        CHECK(bbp_mod_post_event(&mod2, "ctrl.value", 10, &v, 1) == BBP_OK);
        memset(&happ, 0, sizeof happ);
        CHECK(bbp_host_poll_slot(&host, 2) == BBP_OK);
        run_ticks(6);
        CHECK(happ.got && happ.last_src == 2);
        CHECK(host_schema_is(&happ, "ctrl.value"));
        CHECK(happ.payload_len == 1 && happ.payload[0] == 42);
    }

    /* ---- 3. application command with reply ---- */
    printf("test_command_reply\n");
    {
        memset(&happ, 0, sizeof happ);
        CHECK(bbp_host_send(&host, 2, "ctrl.get", 8, NULL, 0) == BBP_OK);
        CHECK(bbp_host_poll_slot(&host, 2) == BBP_OK);
        run_ticks(6);
        CHECK(happ.got && host_schema_is(&happ, "ctrl.value"));
        CHECK(happ.payload_len == 1 && happ.payload[0] == 7);   /* a2.value */
    }

    /* ---- 4. unknown addressed schema -> bbp.error UNKNOWN_SCHEMA + echo ---- */
    printf("test_unknown_schema\n");
    {
        memset(&happ, 0, sizeof happ);
        happ.last_err_class = 0xFF;
        CHECK(bbp_host_send(&host, 2, "no.such", 7, NULL, 0) == BBP_OK);
        CHECK(bbp_host_poll_slot(&host, 2) == BBP_OK);
        run_ticks(6);
        CHECK(happ.got && host_schema_is(&happ, BBP_SCHEMA_ERROR));
        CHECK(happ.last_err_class == BBP_ERRC_UNKNOWN_SCHEMA);
        CHECK(happ.payload_len == 1 + 7);                       /* class + echoed schema */
        CHECK(memcmp(&happ.payload[1], "no.such", 7) == 0);
    }

    /* ---- 5. broadcast transport reaches every module, no reply ---- */
    printf("test_broadcast\n");
    {
        a2.got_start = false; a4.got_start = false;
        memset(&happ, 0, sizeof happ);
        CHECK(bbp_host_broadcast(&host, BBP_SCHEMA_START, 9, NULL, 0) == BBP_OK);
        run_ticks(3);
        CHECK(a2.got_start && a4.got_start);
        CHECK(!happ.got);   /* broadcasts get no response */
    }

    /* ---- 6. polling an empty queue yields silence ---- */
    printf("test_silent_poll\n");
    {
        memset(&happ, 0, sizeof happ);
        CHECK(bbp_host_poll_slot(&host, 2) == BBP_OK);
        run_ticks(5);
        CHECK(!happ.got);
        CHECK(mod2.polls_served > 0);
    }

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
