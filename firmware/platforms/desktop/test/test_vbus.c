/*
 * test_vbus.c — integration test for the desktop virtual-bus platform.
 *
 * Wires a host and two modules onto one virtual bus and exercises directed
 * messaging, broadcast, half-duplex self-exclusion, the virtual clock, and
 * the bus diagnostics. No hardware, no threads.
 *
 *   make test   (from firmware/platforms/desktop)
 */
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

/* Pump a port and poll one packet; returns true and fills *out if one arrived. */
static bool pump_poll(bbp_vport_t *port, bbp_t *b, bbp_packet_t *out)
{
    bbp_vport_pump(port, b);
    return bbp_poll(b, out) == BBP_OK;
}

/* ---- directed request/response between host and a module ---- */
static void test_directed_exchange(void)
{
    bbp_vbus_t bus;
    bbp_vport_t hp, m3p, m5p;
    bbp_t host, m3, m5;
    bbp_packet_t pkt;

    printf("test_directed_exchange\n");
    bbp_vbus_init(&bus);
    CHECK(bbp_vbus_attach(&bus, &hp,  0) == BBP_OK);   /* host: no slot */
    CHECK(bbp_vbus_attach(&bus, &m3p, 3) == BBP_OK);
    CHECK(bbp_vbus_attach(&bus, &m5p, 5) == BBP_OK);
    CHECK(bbp_init(&host, &hp.plat,  BBP_HOST_ADDR) == BBP_OK);
    CHECK(bbp_init(&m3,   &m3p.plat, 3) == BBP_OK);
    CHECK(bbp_init(&m5,   &m5p.plat, 5) == BBP_OK);

    /* host -> slot 3 */
    CHECK(bbp_encode(&host, 3, "bbp.poll", 8, NULL, 0) == BBP_OK);
    CHECK(bbp_transmit(&host) == BBP_OK);

    /* half-duplex: the sender does not receive its own frame */
    bbp_vport_pump(&hp, &host);
    CHECK(bbp_poll(&host, &pkt) == BBP_ERR_NO_PACKET);

    /* the addressed module receives it ... */
    CHECK(pump_poll(&m3p, &m3, &pkt));
    CHECK(pkt.src == BBP_HOST_ADDR && pkt.dest == 3);
    CHECK(bbp_schema_eq(&pkt, "bbp.poll"));

    /* ... and so does the other module (no hardware address filter on a bus;
       acting on dest is a policy-layer decision above the core) */
    CHECK(pump_poll(&m5p, &m5, &pkt));
    CHECK(pkt.dest == 3);

    /* slot 3 replies to the host */
    {
        const uint8_t body[] = { 0x01, 0x02 };
        CHECK(bbp_encode(&m3, BBP_HOST_ADDR, "bbp.info", 8, body, 2) == BBP_OK);
        CHECK(bbp_transmit(&m3) == BBP_OK);
    }
    CHECK(pump_poll(&hp, &host, &pkt));
    CHECK(pkt.src == 3 && pkt.dest == BBP_HOST_ADDR);
    CHECK(bbp_schema_eq(&pkt, "bbp.info"));
    CHECK(pkt.payload_len == 2 && pkt.payload[0] == 0x01 && pkt.payload[1] == 0x02);

    CHECK(bus.collision == false);
    CHECK(bus.rxq_drops == 0);
    CHECK(bus.bytes_carried > 0);
}

/* ---- broadcast reaches every module ---- */
static void test_broadcast(void)
{
    bbp_vbus_t bus;
    bbp_vport_t hp, m1p, m2p;
    bbp_t host, m1, m2;
    bbp_packet_t pkt;

    printf("test_broadcast\n");
    bbp_vbus_init(&bus);
    bbp_vbus_attach(&bus, &hp,  0);
    bbp_vbus_attach(&bus, &m1p, 1);
    bbp_vbus_attach(&bus, &m2p, 2);
    bbp_init(&host, &hp.plat,  BBP_HOST_ADDR);
    bbp_init(&m1,   &m1p.plat, 1);
    bbp_init(&m2,   &m2p.plat, 2);

    CHECK(bbp_encode(&host, BBP_BROADCAST, "bbp.start", 9, NULL, 0) == BBP_OK);
    CHECK(bbp_transmit(&host) == BBP_OK);

    CHECK(pump_poll(&m1p, &m1, &pkt));
    CHECK(pkt.dest == BBP_BROADCAST && bbp_schema_eq(&pkt, "bbp.start"));
    CHECK(pump_poll(&m2p, &m2, &pkt));
    CHECK(pkt.dest == BBP_BROADCAST && bbp_schema_eq(&pkt, "bbp.start"));
}

/* ---- read_slot reports the configured slot; the host reports 0 ---- */
static void test_read_slot_and_clock(void)
{
    bbp_vbus_t bus;
    bbp_vport_t hp, m4p;

    printf("test_read_slot_and_clock\n");
    bbp_vbus_init(&bus);
    bbp_vbus_attach(&bus, &hp,  0);
    bbp_vbus_attach(&bus, &m4p, 4);

    CHECK(hp.plat.read_slot(hp.plat.ctx) == 0);
    CHECK(m4p.plat.read_slot(m4p.plat.ctx) == 4);

    CHECK(hp.plat.millis(hp.plat.ctx) == 0);
    bbp_vbus_advance(&bus, 250);
    CHECK(hp.plat.millis(hp.plat.ctx) == 250);
    CHECK(m4p.plat.millis(m4p.plat.ctx) == 250);   /* one clock, shared */
    bbp_vbus_advance(&bus, 1000);
    CHECK(hp.plat.millis(hp.plat.ctx) == 1250);
}

/* ---- attaching past capacity is reported, not silently dropped ---- */
static void test_bus_full(void)
{
    bbp_vbus_t bus;
    bbp_vport_t ports[BBP_VBUS_MAX_PORTS + 1];
    unsigned i;
    bbp_err_t rc = BBP_OK;

    printf("test_bus_full\n");
    bbp_vbus_init(&bus);
    for (i = 0; i < BBP_VBUS_MAX_PORTS; i++) {
        CHECK(bbp_vbus_attach(&bus, &ports[i], 0) == BBP_OK);
    }
    rc = bbp_vbus_attach(&bus, &ports[BBP_VBUS_MAX_PORTS], 0);
    CHECK(rc == BBP_ERR_PLATFORM);
}

int main(void)
{
    test_directed_exchange();
    test_broadcast();
    test_read_slot_and_clock();
    test_bus_full();

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
