/*
 * bbp_desktop.h — desktop virtual-bus platform for BBP. No hardware required.
 *
 * Models an RS-485 half-duplex multidrop bus entirely in-process: every node
 * attaches to one bbp_vbus_t, and a byte transmitted by any node is delivered
 * to the RX queue of every OTHER attached node. A transmitter does not hear
 * itself (the line driver is on while it sends), exactly like the real bus.
 *
 * The core's RX path is pull-driven (the application calls bbp_receive_byte),
 * so delivered bytes sit in each port's queue until you drain them with
 * bbp_vport_pump(). Time is virtual and deterministic: the bus owns a
 * millisecond clock that advances only via bbp_vbus_advance(); millis() reads
 * it. Address filtering is NOT done here — like real hardware, every node sees
 * every well-formed frame; deciding which to act on is a policy-layer job.
 *
 * Single-threaded, not reentrant. For tests and host-side simulation only.
 *
 * Usage:
 *     bbp_vbus_t  bus;   bbp_vbus_init(&bus);
 *     bbp_vport_t hp, mp; bbp_t host, mod;
 *     bbp_vbus_attach(&bus, &hp, 0);            // host: no slot pin
 *     bbp_vbus_attach(&bus, &mp, 3);            // module in slot 3
 *     bbp_init(&host, &hp.plat, BBP_HOST_ADDR);
 *     bbp_init(&mod,  &mp.plat, 3);
 *     bbp_encode(&host, 3, "bbp.ping", 8, NULL, 0);
 *     bbp_transmit(&host);                      // bytes land in mp's queue
 *     bbp_vport_pump(&mp, &mod);                // feed them into the assembler
 *     bbp_packet_t pkt; bbp_poll(&mod, &pkt);   // module sees the frame
 */
#ifndef BBP_DESKTOP_H
#define BBP_DESKTOP_H

#include "bbp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BBP_VBUS_MAX_PORTS
#define BBP_VBUS_MAX_PORTS 8u      /* host + up to 6 slots, with headroom */
#endif
#ifndef BBP_VPORT_RXQ_LEN
#define BBP_VPORT_RXQ_LEN  1024u   /* per-node received-byte ring capacity */
#endif

typedef struct bbp_vport bbp_vport_t;

typedef struct bbp_vbus {
    bbp_vport_t *ports[BBP_VBUS_MAX_PORTS];
    uint8_t      n_ports;
    uint32_t     now_ms;            /* virtual clock; advanced by bbp_vbus_advance */
    uint8_t      drivers_active;    /* ports currently asserting DE */
    /* diagnostics */
    uint32_t     bytes_carried;     /* total byte-deliveries across the medium */
    uint32_t     rxq_drops;         /* bytes dropped because an RX ring was full */
    bool         collision;         /* two drivers were enabled at once */
} bbp_vbus_t;

struct bbp_vport {
    bbp_vbus_t    *bus;
    bbp_platform_t plat;            /* pass &plat to bbp_init */
    uint8_t        slot;            /* value read_slot() reports; 0 = none/host */
    bool           de;              /* this port's driver-enable state */
    /* received-byte ring */
    uint8_t        rxq[BBP_VPORT_RXQ_LEN];
    uint16_t       head, tail, count;
};

/* Reset a bus to the empty state (no ports, clock at 0). */
void bbp_vbus_init(bbp_vbus_t *bus);

/* Attach a port to the bus and populate port->plat. `slot` is what read_slot()
   will report (1..6 for a module, 0 for the host). Returns BBP_ERR_NULL_ARG on
   NULL args, BBP_ERR_PLATFORM if the bus is full, else BBP_OK. After this,
   call bbp_init(b, &port->plat, addr). */
bbp_err_t bbp_vbus_attach(bbp_vbus_t *bus, bbp_vport_t *port, uint8_t slot);

/* Drain this port's queued bytes into core instance b (one bbp_receive_byte per
   byte). Returns the number of bytes fed. */
size_t bbp_vport_pump(bbp_vport_t *port, bbp_t *b);

/* Advance the bus virtual clock by ms milliseconds. */
void bbp_vbus_advance(bbp_vbus_t *bus, uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* BBP_DESKTOP_H */
