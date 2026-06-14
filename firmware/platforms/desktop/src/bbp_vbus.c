/*
 * bbp_vbus.c — desktop virtual-bus platform implementation.
 *
 * The platform callbacks below are handed to bbp_init via port->plat. tx()
 * delivers bytes to peers, set_driver_enable() models the half-duplex line
 * driver (and flags collisions), millis() reads the bus virtual clock, and
 * read_slot() returns the port's configured slot.
 */
#include "bbp_desktop.h"
#include <string.h>

/* ---- per-port received-byte ring ---- */
static bool vport_push(bbp_vport_t *p, uint8_t byte)
{
    if (p->count >= BBP_VPORT_RXQ_LEN) {
        return false;
    }
    p->rxq[p->head] = byte;
    p->head = (uint16_t)((p->head + 1u) % BBP_VPORT_RXQ_LEN);
    p->count++;
    return true;
}

/* ---- platform callbacks (ctx is the bbp_vport_t) ---- */
static void vp_tx(void *ctx, const uint8_t *buf, size_t len)
{
    bbp_vport_t *self = (bbp_vport_t *)ctx;
    bbp_vbus_t  *bus  = self->bus;
    size_t i;
    uint8_t k;

    for (i = 0; i < len; i++) {
        for (k = 0; k < bus->n_ports; k++) {
            bbp_vport_t *peer = bus->ports[k];
            if (peer == self) {
                continue;            /* half-duplex: a node never hears itself */
            }
            if (vport_push(peer, buf[i])) {
                bus->bytes_carried++;
            } else {
                bus->rxq_drops++;    /* peer hasn't pumped; queue overflowed */
            }
        }
    }
}

static void vp_set_de(void *ctx, bool on)
{
    bbp_vport_t *self = (bbp_vport_t *)ctx;
    bbp_vbus_t  *bus  = self->bus;

    if (on && !self->de) {
        self->de = true;
        bus->drivers_active++;
        if (bus->drivers_active > 1u) {
            bus->collision = true;   /* two drivers on the line at once */
        }
    } else if (!on && self->de) {
        self->de = false;
        if (bus->drivers_active > 0u) {
            bus->drivers_active--;
        }
    }
}

static uint32_t vp_millis(void *ctx)
{
    return ((bbp_vport_t *)ctx)->bus->now_ms;
}

static uint8_t vp_read_slot(void *ctx)
{
    return ((bbp_vport_t *)ctx)->slot;
}

static int vp_rx(void *ctx)
{
    bbp_vport_t *self = (bbp_vport_t *)ctx;
    uint8_t byte;
    if (self->count == 0u) {
        return -1;                   /* nothing queued */
    }
    byte = self->rxq[self->tail];
    self->tail = (uint16_t)((self->tail + 1u) % BBP_VPORT_RXQ_LEN);
    self->count--;
    return (int)byte;
}

/* ---- public API ---- */
void bbp_vbus_init(bbp_vbus_t *bus)
{
    if (bus != NULL) {
        memset(bus, 0, sizeof *bus);
    }
}

bbp_err_t bbp_vbus_attach(bbp_vbus_t *bus, bbp_vport_t *port, uint8_t slot)
{
    if (bus == NULL || port == NULL) {
        return BBP_ERR_NULL_ARG;
    }
    if (bus->n_ports >= BBP_VBUS_MAX_PORTS) {
        return BBP_ERR_PLATFORM;     /* bus full; raise BBP_VBUS_MAX_PORTS */
    }
    memset(port, 0, sizeof *port);
    port->bus  = bus;
    port->slot = slot;
    port->plat.tx                = vp_tx;
    port->plat.set_driver_enable = vp_set_de;
    port->plat.millis            = vp_millis;
    port->plat.read_slot         = vp_read_slot;
    port->plat.rx                = vp_rx;
    port->plat.ctx               = port;
    bus->ports[bus->n_ports++] = port;
    return BBP_OK;
}

size_t bbp_vport_pump(bbp_vport_t *port, bbp_t *b)
{
    size_t n = 0;
    if (port == NULL || b == NULL) {
        return 0;
    }
    while (port->count > 0u) {
        uint8_t byte = port->rxq[port->tail];
        port->tail = (uint16_t)((port->tail + 1u) % BBP_VPORT_RXQ_LEN);
        port->count--;
        bbp_receive_byte(b, byte);
        n++;
    }
    return n;
}

void bbp_vbus_advance(bbp_vbus_t *bus, uint32_t ms)
{
    if (bus != NULL) {
        bus->now_ms += ms;
    }
}
