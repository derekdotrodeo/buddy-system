/*
 * rp2040-peripheral — a Buddy Card (module) on an RP2040 (Pico SDK).
 *
 * A "CTL-1" control module: reads its slot from the SLOT pin, then reports a
 * potentiometer (controls.pot) and a button (controls.button) as events,
 * drained by the host's polls. Answers bbp.identify/bbp.poll automatically via
 * the policy layer.
 *
 * Wiring (defaults below; change to taste):
 *   GP4  -> RS-485 transceiver DI  (UART1 TX)
 *   GP5  <- RS-485 transceiver RO  (UART1 RX)
 *   GP6  -> RS-485 transceiver DE and ~RE (tied together)
 *   GP26 <- SLOT analog id   (ADC0)
 *   GP27 <- potentiometer wiper (ADC1)
 *   GP15 <- button to GND (internal pull-up; active low)
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "bbp_policy.h"
#include "bbp_rp2040.h"
#include "bbp_schemas.h"

#define BBP_UART        uart1
#define BBP_PIN_TX      4u
#define BBP_PIN_RX      5u
#define BBP_PIN_DE      6u
#define SLOT_ADC_INPUT  0u      /* GP26 = ADC0 */
#define POT_ADC_INPUT   1u      /* GP27 = ADC1 */
#define BTN_PIN         15u

static bbp_rp2040_t port;
static bbp_t        core;
static bbp_mod_t    mod;
static const bbp_identity_t identity = { "BUDDY", "CTL-1", 1, 0, 1 };

typedef struct { int running; } app_t;
static app_t app;

static bool on_packet(bbp_mod_t *m, const bbp_packet_t *pkt, void *a)
{
    app_t *st = (app_t *)a;
    (void)m;
    if (pkt->dest == BBP_BROADCAST) {        /* transport/sync, no reply */
        if (bbp_schema_eq(pkt, BBP_SCHEMA_START))     st->running = 1;
        else if (bbp_schema_eq(pkt, BBP_SCHEMA_STOP))  st->running = 0;
        return true;
    }
    return false;   /* no addressed commands -> policy answers UNKNOWN_SCHEMA */
}

int main(void)
{
    bbp_rp2040_config_t cfg = {
        .uart = BBP_UART, .baud = 1000000u,
        .pin_tx = BBP_PIN_TX, .pin_rx = BBP_PIN_RX, .pin_de = BBP_PIN_DE,
        .use_slot_adc = true, .slot_adc_input = SLOT_ADC_INPUT,
    };
    uint16_t last_pot = 0;
    bool     last_btn = false;
    absolute_time_t next;

    stdio_init_all();
    bbp_rp2040_init(&port, &cfg);

    /* extra hardware beyond the platform: a pot on another ADC and a button */
    adc_gpio_init(26u + POT_ADC_INPUT);
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    bbp_init(&core, &port.plat, 0);          /* address adopted from SLOT */
    if (bbp_mod_init(&mod, &core, &identity, on_packet, &app) != BBP_OK) {
        printf("bad SLOT reading; check wiring\n");
        while (true) { tight_loop_contents(); }
    }
    printf("CTL-1 ready in slot %u\n", (unsigned)mod.slot);

    next = make_timeout_time_ms(20);
    while (true) {
        bbp_mod_task(&mod);                  /* service the bus every loop */

        if (absolute_time_diff_us(get_absolute_time(), next) <= 0) {
            int d;
            uint16_t pot;
            bool btn;

            next = make_timeout_time_ms(20);

            /* pot: raw 12-bit value as controls.pot (u16), posted on a
               meaningful change (deadband against ADC jitter) */
            adc_select_input(POT_ADC_INPUT);
            pot = (uint16_t)adc_read();          /* 0..4095 */
            d = (int)pot - (int)last_pot;
            if (d > 16 || d < -16) {
                uint8_t pe[BBP_POT_LEN];
                bbp_pot_encode(pe, 0, pot);
                bbp_mod_post_event(&mod, BBP_SCHEMA_POT,
                                   (uint8_t)(sizeof BBP_SCHEMA_POT - 1), pe, BBP_POT_LEN);
                last_pot = pot;
            }

            /* button: active low, posted on edge as controls.button */
            btn = !gpio_get(BTN_PIN);
            if (btn != last_btn) {
                uint8_t be[BBP_BUTTON_LEN];
                bbp_button_encode(be, 0, btn);
                bbp_mod_post_event(&mod, BBP_SCHEMA_BUTTON,
                                   (uint8_t)(sizeof BBP_SCHEMA_BUTTON - 1), be, BBP_BUTTON_LEN);
                last_btn = btn;
            }
        }
    }
    return 0;
}
