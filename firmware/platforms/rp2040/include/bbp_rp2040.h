/*
 * bbp_rp2040.h — Raspberry Pi RP2040 platform for BBP (Pico SDK).
 *
 * Implements the bbp_platform_t hooks on RP2040 hardware:
 *   - tx           : UART write, blocking until the last stop bit is on the wire
 *   - driver enable: a GPIO driving the RS-485 transceiver's DE and ~RE (tied)
 *   - millis       : to_ms_since_boot(get_absolute_time())
 *   - read_slot    : ADC read of the SLOT pin -> slot 1..6 (modules only)
 *   - rx           : non-blocking pop from an interrupt-fed RX ring
 *
 * One transceiver pin drives DE and ~RE together: high = transmit (and the
 * receiver is muted, so a node never hears itself); low = receive.
 *
 * Requires the Pico SDK (pico_stdlib, hardware_uart, hardware_gpio,
 * hardware_adc, hardware_irq). Single-core use (core0); not reentrant.
 */
#ifndef BBP_RP2040_H
#define BBP_RP2040_H

#include "bbp.h"
#include "hardware/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BBP_RP2040_RXQ_LEN
#define BBP_RP2040_RXQ_LEN 512u    /* must be a power-of-two-friendly size */
#endif

typedef struct {
    uart_inst_t *uart;            /* uart0 or uart1 */
    uint32_t     baud;            /* BS-SPEC-100 §6.4: 1000000 */
    uint32_t     pin_tx;          /* UART TX GPIO */
    uint32_t     pin_rx;          /* UART RX GPIO */
    uint32_t     pin_de;          /* RS-485 DE and ~RE (tied) GPIO */
    bool         use_slot_adc;    /* true on a module, false on the host */
    uint32_t     slot_adc_input;  /* ADC input 0..3 (GP26..29) wired to SLOT */
} bbp_rp2040_config_t;

typedef struct bbp_rp2040 {
    bbp_platform_t    plat;       /* pass &plat to bbp_init() */
    uart_inst_t      *uart;
    uint32_t          pin_de;
    bool              use_slot_adc;
    uint32_t          slot_adc_input;
    /* single-producer (IRQ) / single-consumer (rx) ring */
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint8_t  rxq[BBP_RP2040_RXQ_LEN];
} bbp_rp2040_t;

/* Configure the UART (8N1 at cfg->baud), the DE GPIO (receive mode), the RX
   interrupt ring, and — for modules — the SLOT ADC; then fill port->plat.
   After this, call bbp_init(core, &port->plat, addr). */
bbp_err_t bbp_rp2040_init(bbp_rp2040_t *port, const bbp_rp2040_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* BBP_RP2040_H */
