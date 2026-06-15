/*
 * demo.c — a small Buddy System app on the desktop virtual bus (no hardware).
 *
 * Builds a host plus two modules — a POT-6 in slot 1 and a BTN-4 in slot 3 —
 * on an in-process RS-485 virtual bus, then:
 *   1. discovers the bus and prints the roster (bbp.identify -> bbp.info),
 *   2. broadcasts bbp.start,
 *   3. polls each populated slot for a few rounds, printing control events,
 *   4. broadcasts bbp.stop.
 *
 * Control events use the common schemas (BS-SPEC-300) via bbp_schemas.h. This
 * is the same policy + core code that runs on real hardware; only the platform
 * (the virtual bus) is swapped for a simulated one.
 *
 *   make run   (from firmware/examples/desktop-demo)
 */
#include "bbp_policy.h"
#include "bbp_desktop.h"
#include "bbp_schemas.h"
#include <stdio.h>

/* ---- module application state ---- */
typedef struct { int started; } mod_state_t;

static bool mod_handler(bbp_mod_t *m, const bbp_packet_t *pkt, void *app)
{
    mod_state_t *s = (mod_state_t *)app;
    (void)m;
    if (pkt->dest == BBP_BROADCAST) {
        if (bbp_schema_eq(pkt, BBP_SCHEMA_START))     { s->started = 1; }
        else if (bbp_schema_eq(pkt, BBP_SCHEMA_STOP)) { s->started = 0; }
        return true;
    }
    return false;   /* these modules expose no addressed commands */
}

/* ---- host application: decode and print what cards report ---- */
static void host_handler(bbp_host_t *h, const bbp_packet_t *pkt, void *app)
{
    uint8_t  idx;
    uint16_t value;
    bool     pressed;
    (void)h; (void)app;

    if (bbp_schema_eq(pkt, BBP_SCHEMA_POT) &&
        bbp_pot_decode(pkt->payload, pkt->payload_len, &idx, &value)) {
        printf("    [slot %u] pot %u -> %5u\n",
               (unsigned)pkt->src, (unsigned)idx, (unsigned)value);
    } else if (bbp_schema_eq(pkt, BBP_SCHEMA_BUTTON) &&
               bbp_button_decode(pkt->payload, pkt->payload_len, &idx, &pressed)) {
        printf("    [slot %u] button %u %s\n",
               (unsigned)pkt->src, (unsigned)idx, pressed ? "DOWN" : "up");
    } else if (bbp_schema_eq(pkt, BBP_SCHEMA_ERROR) && pkt->payload_len >= 1) {
        printf("    [slot %u] bbp.error class %u\n",
               (unsigned)pkt->src, (unsigned)pkt->payload[0]);
    } else {
        printf("    [slot %u] %.*s (%u bytes)\n",
               (unsigned)pkt->src, (int)pkt->schema_len, pkt->schema,
               (unsigned)pkt->payload_len);
    }
}

/* ---- cooperative scheduler: one task pass per node, then advance the clock ---- */
static bbp_host_t *g_host;
static bbp_mod_t  *g_mods[2];
static bbp_vbus_t *g_bus;

static void ticks(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        bbp_host_task(g_host);
        bbp_mod_task(g_mods[0]);
        bbp_mod_task(g_mods[1]);
        bbp_vbus_advance(g_bus, 1);   /* 1 ms per tick */
    }
}

int main(void)
{
    bbp_vbus_t  bus;
    bbp_vport_t hp, pot_port, btn_port;
    bbp_t       hc, pot_c, btn_c;
    bbp_host_t  host;
    bbp_mod_t   pot, btn;
    mod_state_t pot_app = { 0 }, btn_app = { 0 };
    const bbp_identity_t pot_id = { "BUDDY", "POT-6", 1, 0, 1 };
    const bbp_identity_t btn_id = { "BUDDY", "BTN-4", 1, 0, 1 };
    int guard, found, round;
    uint8_t s;

    /* bus + nodes: host (no slot), POT-6 in slot 1, BTN-4 in slot 3 */
    bbp_vbus_init(&bus);
    bbp_vbus_attach(&bus, &hp, 0);
    bbp_vbus_attach(&bus, &pot_port, 1);
    bbp_vbus_attach(&bus, &btn_port, 3);
    bbp_init(&hc, &hp.plat, BBP_HOST_ADDR);
    bbp_init(&pot_c, &pot_port.plat, 0);   /* address adopted from the slot pin */
    bbp_init(&btn_c, &btn_port.plat, 0);

    bbp_host_init(&host, &hc, host_handler, NULL);
    bbp_mod_init(&pot, &pot_c, &pot_id, mod_handler, &pot_app);
    bbp_mod_init(&btn, &btn_c, &btn_id, mod_handler, &btn_app);

    g_host = &host; g_mods[0] = &pot; g_mods[1] = &btn; g_bus = &bus;

    printf("Buddy System - desktop demo\n\n");

    /* 1. discovery */
    printf("Discovering bus...\n");
    bbp_host_start_discovery(&host);
    guard = 0;
    while (bbp_host_discovery_busy(&host) && guard++ < 1000) {
        ticks(1);
    }
    found = 0;
    for (s = BBP_SLOT_MIN; s <= BBP_SLOT_MAX; s++) {
        const bbp_slot_rec_t *r = bbp_host_slot(&host, s);
        if (r != NULL && r->present) {
            printf("  slot %u: %.*s %.*s v%u.%u (%u slot%s)\n",
                   (unsigned)s,
                   (int)r->info.vendor_len, r->info.vendor,
                   (int)r->info.product_len, r->info.product,
                   (unsigned)r->info.version_major, (unsigned)r->info.version_minor,
                   (unsigned)r->info.slot_count, r->info.slot_count == 1 ? "" : "s");
            found++;
        }
    }
    printf("%d module(s) found.\n\n", found);

    /* 2. start */
    printf("Broadcast: bbp.start\n");
    bbp_host_broadcast(&host, BBP_SCHEMA_START, 9, NULL, 0);
    ticks(3);
    printf("  POT-6 running=%d  BTN-4 running=%d\n\n", pot_app.started, btn_app.started);

    /* 3. poll for events */
    printf("Polling for events (8 rounds)...\n");
    for (round = 0; round < 8; round++) {
        /* simulate hardware: the POT-6 sweeps a pot each round (full 16-bit
           range); the BTN-4 reports a button every third round */
        uint8_t pe[BBP_POT_LEN];
        bbp_pot_encode(pe, (uint8_t)(round % 6), (uint16_t)(round * 4096));
        bbp_mod_post_event(&pot, BBP_SCHEMA_POT,
                           (uint8_t)(sizeof BBP_SCHEMA_POT - 1), pe, BBP_POT_LEN);
        if (round % 3 == 0) {
            uint8_t be[BBP_BUTTON_LEN];
            bbp_button_encode(be, (uint8_t)(round % 4), ((round / 3) % 2) != 0);
            bbp_mod_post_event(&btn, BBP_SCHEMA_BUTTON,
                               (uint8_t)(sizeof BBP_SCHEMA_BUTTON - 1), be, BBP_BUTTON_LEN);
        }
        /* host drains one packet per populated slot */
        for (s = BBP_SLOT_MIN; s <= BBP_SLOT_MAX; s++) {
            const bbp_slot_rec_t *r = bbp_host_slot(&host, s);
            if (r != NULL && r->present) {
                bbp_host_poll_slot(&host, s);
            }
        }
        ticks(5);   /* let responses arrive and print */
    }

    /* 4. stop */
    printf("\nBroadcast: bbp.stop\n");
    bbp_host_broadcast(&host, BBP_SCHEMA_STOP, 8, NULL, 0);
    ticks(3);
    printf("  POT-6 running=%d  BTN-4 running=%d\n\n", pot_app.started, btn_app.started);

    printf("Done.\n");
    return 0;
}
