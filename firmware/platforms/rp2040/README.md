# BBP RP2040 platform

Implements the BBP `bbp_platform_t` on RP2040 hardware using the [Pico SDK](https://github.com/raspberrypi/pico-sdk):

| Hook | RP2040 implementation |
| ---- | --------------------- |
| `tx` | `uart_write_blocking` + `uart_tx_wait_blocking` (blocks until the last stop bit leaves the wire, so DE can drop safely) |
| `set_driver_enable` | a GPIO driving the RS-485 transceiver's `DE` and `~RE` (tied) |
| `millis` | `to_ms_since_boot(get_absolute_time())` |
| `read_slot` | ADC read of the SLOT pin → slot 1..6 (BS-SPEC-100 §6.3 thresholds) |
| `rx` | non-blocking pop from a UART-RX-interrupt-fed ring buffer |

`DE`/`~RE` are tied to one GPIO: **high = transmit** (receiver muted, so a node never hears itself), **low = receive** — matching the half-duplex model in BS-SPEC-200 §4.

## Hardware

A 3.3 V RS-485 transceiver (e.g. MAX3485, THVD1450, SP3485) wired to the Buddy Bus
`BUS A`/`BUS B`. Default pins (see the example `main.c` files to change them):

| RP2040 | Signal |
| ------ | ------ |
| GP4 | UART1 TX → transceiver `DI` |
| GP5 | UART1 RX ← transceiver `RO` |
| GP6 | transceiver `DE` + `~RE` (tied) |
| GP26 (ADC0) | SLOT analog id (modules only) |

> Termination and biasing live on the Buddy Base, not the card (BS-SPEC-100 §6.2).

## Build (examples)

The platform builds only under the Pico SDK; the standalone `Makefile` here builds
just the host-testable SLOT-mapping unit test.

```sh
export PICO_SDK_PATH=/path/to/pico-sdk

# host (bus master)
cd ../../examples/rp2040-host
cmake -B build -S . && cmake --build build       # -> build/bbp_host.uf2

# peripheral (a CTL-1 module)
cd ../../examples/rp2040-peripheral
cmake -B build -S . && cmake --build build       # -> build/bbp_peripheral.uf2
```

Flash by holding BOOTSEL, plugging in the board, and copying the `.uf2`. The
console (discovery roster, events) appears on the USB serial port at any baud.

## Tests / verification status

- **SLOT mapping** (`bbp_rp2040_slot.c`) is pure C and unit-tested on the host:
  `make test`.
- The **SDK-dependent platform** (`bbp_rp2040.c`) and the two examples are written
  against the Pico SDK API but have **not yet been built with the SDK or run on
  hardware** — they need a board to validate UART/DE turnaround timing and the RX
  interrupt path. Bring-up notes:
  - confirm both nodes agree on 1 Mbit/s and the DE GPIO polarity,
  - scope the DE line vs. TX to verify the turnaround (DE must stay asserted
    until the last stop bit — handled by `uart_tx_wait_blocking`),
  - the host's discovery timeout defaults to 5 ms; widen via
    `bbp_host_set_timeout` if a slow module misses it.
