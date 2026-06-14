/*
 * bbp_rp2040.c — RP2040 platform implementation (Pico SDK).
 */
#include "bbp_rp2040.h"
#include "bbp_rp2040_slot.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include <string.h>

/* Registry so the per-UART IRQ handlers can find their port (RP2040 has two
   UARTs, each with its own IRQ vector). */
static bbp_rp2040_t *g_ports[2];

static void drain(bbp_rp2040_t *p)
{
    while (uart_is_readable(p->uart)) {
        uint8_t  b  = (uint8_t)uart_getc(p->uart);
        uint16_t nh = (uint16_t)((p->head + 1u) % BBP_RP2040_RXQ_LEN);
        if (nh != p->tail) {
            p->rxq[p->head] = b;
            p->head = nh;
        }
        /* else: ring full — drop the byte; the CRC will reject any torn frame */
    }
}

static void on_uart0(void) { if (g_ports[0] != NULL) drain(g_ports[0]); }
static void on_uart1(void) { if (g_ports[1] != NULL) drain(g_ports[1]); }

/* ---- platform callbacks ---- */
static void rp_tx(void *ctx, const uint8_t *buf, size_t len)
{
    bbp_rp2040_t *p = (bbp_rp2040_t *)ctx;
    uart_write_blocking(p->uart, buf, len);
    uart_tx_wait_blocking(p->uart);   /* block until the shift register is empty */
}

static void rp_set_de(void *ctx, bool on)
{
    bbp_rp2040_t *p = (bbp_rp2040_t *)ctx;
    gpio_put(p->pin_de, on);          /* high = drive the bus (RX muted) */
}

static uint32_t rp_millis(void *ctx)
{
    (void)ctx;
    return to_ms_since_boot(get_absolute_time());
}

static uint8_t rp_read_slot(void *ctx)
{
    bbp_rp2040_t *p = (bbp_rp2040_t *)ctx;
    if (!p->use_slot_adc) {
        return 0u;
    }
    adc_select_input(p->slot_adc_input);
    return bbp_rp2040_slot_from_raw((uint16_t)adc_read(), 4095u);
}

static int rp_rx(void *ctx)
{
    bbp_rp2040_t *p = (bbp_rp2040_t *)ctx;
    uint8_t b;
    if (p->head == p->tail) {
        return -1;                    /* empty */
    }
    b = p->rxq[p->tail];
    p->tail = (uint16_t)((p->tail + 1u) % BBP_RP2040_RXQ_LEN);
    return (int)b;
}

bbp_err_t bbp_rp2040_init(bbp_rp2040_t *port, const bbp_rp2040_config_t *cfg)
{
    uint32_t idx;
    int      irqn;

    if (port == NULL || cfg == NULL || cfg->uart == NULL) {
        return BBP_ERR_NULL_ARG;
    }

    memset(port, 0, sizeof *port);
    port->uart           = cfg->uart;
    port->pin_de         = cfg->pin_de;
    port->use_slot_adc   = cfg->use_slot_adc;
    port->slot_adc_input = cfg->slot_adc_input;

    /* RS-485 DE/~RE: start in receive mode (low) */
    gpio_init(cfg->pin_de);
    gpio_set_dir(cfg->pin_de, GPIO_OUT);
    gpio_put(cfg->pin_de, 0);

    /* UART, 8N1, no flow control */
    uart_init(cfg->uart, cfg->baud);
    gpio_set_function(cfg->pin_tx, GPIO_FUNC_UART);
    gpio_set_function(cfg->pin_rx, GPIO_FUNC_UART);
    uart_set_format(cfg->uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(cfg->uart, true);
    uart_set_hw_flow(cfg->uart, false, false);

    /* RX interrupt feeds the ring */
    idx = uart_get_index(cfg->uart);
    g_ports[idx] = port;
    irqn = (cfg->uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irqn, (idx == 0u) ? on_uart0 : on_uart1);
    irq_set_enabled(irqn, true);
    uart_set_irq_enables(cfg->uart, true, false);   /* RX + RX-timeout, no TX */

    /* SLOT ADC (modules only) */
    if (cfg->use_slot_adc) {
        adc_init();
        adc_gpio_init(26u + cfg->slot_adc_input);
    }

    port->plat.tx                = rp_tx;
    port->plat.set_driver_enable = rp_set_de;
    port->plat.millis            = rp_millis;
    port->plat.read_slot         = rp_read_slot;
    port->plat.rx                = rp_rx;
    port->plat.ctx               = port;
    return BBP_OK;
}
