/*
 * rp2040-host — a Buddy System host on an RP2040 (Pico SDK).
 *
 * Discovers the bus, prints the roster over USB serial, then round-robin polls
 * every populated slot every 10 ms and prints the control events it receives.
 *
 * Wiring (defaults below; change to taste):
 *   GP4  -> RS-485 transceiver DI  (UART1 TX)
 *   GP5  <- RS-485 transceiver RO  (UART1 RX)
 *   GP6  -> RS-485 transceiver DE and ~RE (tied together)
 *   USB  -> host PC for the serial console (stdio over USB CDC)
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "bbp_policy.h"
#include "bbp_rp2040.h"
#include "bbp_schemas.h"

#define BBP_UART    uart1
#define BBP_PIN_TX  4u
#define BBP_PIN_RX  5u
#define BBP_PIN_DE  6u

static bbp_rp2040_t port;
static bbp_t        core;
static bbp_host_t   host;

static void on_packet(bbp_host_t *h, const bbp_packet_t *pkt, void *app)
{
    uint8_t  idx;
    uint16_t value;
    bool     pressed;
    (void)h; (void)app;
    if (bbp_schema_eq(pkt, BBP_SCHEMA_POT) &&
        bbp_pot_decode(pkt->payload, pkt->payload_len, &idx, &value)) {
        printf("[slot %u] pot %u -> %u\n",
               (unsigned)pkt->src, (unsigned)idx, (unsigned)value);
    } else if (bbp_schema_eq(pkt, BBP_SCHEMA_BUTTON) &&
               bbp_button_decode(pkt->payload, pkt->payload_len, &idx, &pressed)) {
        printf("[slot %u] button %u %s\n",
               (unsigned)pkt->src, (unsigned)idx, pressed ? "DOWN" : "up");
    } else if (bbp_schema_eq(pkt, BBP_SCHEMA_ERROR) && pkt->payload_len >= 1) {
        printf("[slot %u] bbp.error class %u\n",
               (unsigned)pkt->src, (unsigned)pkt->payload[0]);
    } else {
        printf("[slot %u] %.*s (%u bytes)\n",
               (unsigned)pkt->src, (int)pkt->schema_len, pkt->schema,
               (unsigned)pkt->payload_len);
    }
}

int main(void)
{
    bbp_rp2040_config_t cfg = {
        .uart = BBP_UART, .baud = 1000000u,
        .pin_tx = BBP_PIN_TX, .pin_rx = BBP_PIN_RX, .pin_de = BBP_PIN_DE,
        .use_slot_adc = false, .slot_adc_input = 0u,
    };
    absolute_time_t next;
    uint8_t s;

    stdio_init_all();
    sleep_ms(2000);                 /* let USB CDC enumerate before printing */
    printf("\nBBP host starting (1 Mbit/s)\n");

    bbp_rp2040_init(&port, &cfg);
    bbp_init(&core, &port.plat, BBP_HOST_ADDR);
    bbp_host_init(&host, &core, on_packet, NULL);

    printf("Discovering...\n");
    bbp_host_start_discovery(&host);
    while (bbp_host_discovery_busy(&host)) {
        bbp_host_task(&host);
    }
    for (s = BBP_SLOT_MIN; s <= BBP_SLOT_MAX; s++) {
        const bbp_slot_rec_t *r = bbp_host_slot(&host, s);
        if (r != NULL && r->present) {
            printf("  slot %u: %.*s %.*s v%u.%u\n", (unsigned)s,
                   (int)r->info.vendor_len, r->info.vendor,
                   (int)r->info.product_len, r->info.product,
                   (unsigned)r->info.version_major, (unsigned)r->info.version_minor);
        }
    }
    printf("Polling...\n");

    s = BBP_SLOT_MIN;
    next = make_timeout_time_ms(10);
    while (true) {
        bbp_host_task(&host);
        if (absolute_time_diff_us(get_absolute_time(), next) <= 0) {
            int n;
            for (n = 0; n < BBP_SLOT_MAX; n++) {     /* next populated slot */
                s = (s >= BBP_SLOT_MAX) ? BBP_SLOT_MIN : (uint8_t)(s + 1u);
                const bbp_slot_rec_t *r = bbp_host_slot(&host, s);
                if (r != NULL && r->present) {
                    bbp_host_poll_slot(&host, s);
                    break;
                }
            }
            next = make_timeout_time_ms(10);
        }
    }
    return 0;
}
