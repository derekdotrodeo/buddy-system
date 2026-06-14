---
title: Buddy Bus Protocol
description: BS-SPEC-200 defines the Buddy Bus Protocol — a host-polled transport bus that carries schema-defined packets between a host and Buddy Cards.
order: 2
section: Specifications
---

The Buddy Bus Protocol (BBP) is a host-polled transport bus that carries schema-defined packets between a host microcontroller and Buddy Cards over the Buddy System backplane.

**BS-SPEC-200** · Version 1.3 Draft · Jun 2026 · Requires [BS-SPEC-100](/docs/technical-spec/)

## 1. Purpose

*Normative*

The Buddy Bus Protocol (BBP) provides communication between a host microcontroller and Buddy Cards connected through the Buddy System backplane. The electrical and mechanical bus it runs on is defined in [BS-SPEC-100](/docs/technical-spec/).

BBP is a **transport**. It defines packet framing, addressing, a host-controlled polling model, and synchronization. It does **not** define what payloads mean — payload interpretation is carried by named **schemas** (§7).

BBP carries **control and event traffic only**. It is not an audio transport, a bulk-data transport, or a streaming protocol.

BBP is organized in three layers:

1. **Transport** — framing, addressing, polling, and CRC (this document).
2. **Schemas** — payload meaning, identified by schema name (§7).
3. **Module implementations** — local queueing, behavior, and data generation, defined by each module.

BBP is designed to:

- Minimize wiring requirements
- Allow smart peripheral modules
- Support experimentation and custom module development
- Remain independent of any specific microcontroller platform
- Avoid requiring centralized capability registries or GUI tooling

BBP is intended for developers comfortable writing firmware.

## 2. Architecture

*Normative*

### 2.1 Host

The user-selected microcontroller mounted to the Buddy Base breadboard — for example a Daisy Seed, Teensy, RP2040, STM32, or Arduino-compatible MCU. The Host:

- Performs discovery
- Controls all bus access
- Polls cards
- Sends and receives packets
- Executes application logic

### 2.2 Buddy Card

An intelligent peripheral. Each Buddy Card shall contain:

- A local MCU
- An ADC input connected to the SLOT pin
- An RS-485 transceiver connected to BUS A / BUS B
- A BBP implementation

### 2.3 Buddy Base

Provides power distribution, Buddy Bus interconnection, RS-485 termination, and slot identification circuitry. The Buddy Base does not perform protocol processing.

### 2.4 Module requirements

To be BBP-capable, a Buddy Card shall provide, at minimum:

- An MCU
- An ADC connected to the SLOT pin
- An RS-485 transceiver connected to BUS A / BUS B

and shall be able to:

- Read its slot identity from the SLOT signal
- Receive BBP packets
- Transmit a response when polled

## 3. Physical Layer

*Normative*

### 3.1 Transport

BBP uses an RS-485 multidrop bus operating in half-duplex. The topology, termination, and electrical characteristics are defined in [BS-SPEC-100](/docs/technical-spec/) §6.

### 3.2 Buddy Bus pinout

BBP assigns the following signals to the Buddy Bus connector. Pin 1 is the leftmost pin when viewed from the front of a Buddy Card.

```
Buddy Bus Pinout — viewed from front of card, pin 1 leftmost (both connectors identical)

  PIN 1     PIN 2      PIN 3      PIN 4     PIN 5     PIN 6
  SLOT      BUS A      BUS B      3V3       5V        GND
  ANALOG ID RS-485 A   RS-485 B   +3.3 V    +5 V      GROUND
```

| Pin | Signal | Function |
| --- | ------ | -------- |
| 1 | SLOT | Analog ID |
| 2 | BUS A | RS-485 A |
| 3 | BUS B | RS-485 B |
| 4 | 3V3 | +3.3 V |
| 5 | 5V | +5 V |
| 6 | GND | Ground |

*FIG 1 — Buddy Bus pinout per BBP v1 · viewed from front of card · pin 1 leftmost · both connectors identical.*

> **Note:** Pin 1 (SLOT) and the bus electrical layer are defined in [BS-SPEC-100](/docs/technical-spec/) §5.1 and §6.3. BBP relies on the SLOT signal for addressing (§6).

### 3.3 Slot identification

Each slot provides a unique analog identification value on the SLOT pin. Each Buddy Card shall:

- Read SLOT during startup
- Determine its slot number
- Use the slot number as its BBP address

## 4. Bus Access

*Normative*

BBP v1 uses a **Host-controlled polling model**. This is the defining characteristic of BBP.

- Only the Host may initiate bus activity.
- A Buddy Card may transmit only after receiving a `bbp.poll` addressed to its slot.
- A Buddy Card may transmit zero or one response packet per poll.
- Buddy Cards shall not initiate transmissions independently.

Because access is host-granted, the bus needs no collision arbitration: at most one device is transmitting at any instant.

### 4.1 Event handling

Cards may generate internal events — potentiometer changes, button presses, MIDI messages, encoder movement. Events are queued locally on the card. The Host drains the queue one packet at a time by polling; each poll yields at most one queued event.

```
  HOST                              BUDDY CARD
  BUS MASTER                        LOCAL MCU
  APPLICATION LOGIC
                                    ┌────────────────────────┐
       ── bbp.poll → SLOT N ─────►  │ EVENT QUEUE            │
                                    │ POT · BTN · MIDI · ENC │
       ◄── one packet, or silence   │                        │
                                    └────────────────────────┘
       HOST INITIATES EVERY EXCHANGE · ONE PACKET PER POLL
```

*FIG 2 — Bus access · cards queue events locally and the Host drains them one packet per poll (§4) · schematic.*

### 4.2 Response ordering

Each card maintains a single outgoing queue holding both protocol responses (such as `bbp.info` and `bbp.error`) and locally generated events. Each `bbp.poll` releases at most one packet from this queue (§4).

Protocol responses are released **before** queued events; within a class, packets are released oldest-first. This keeps discovery responses and error reports from being delayed behind a burst of events. When the queue is empty, the card transmits nothing.

### 4.3 Poll response timeout

After issuing a `bbp.poll` (or any addressed command that solicits a response), the Host waits a bounded turnaround interval for the response to begin. If none begins within that interval, the Host treats the slot as silent — an empty slot during discovery (§9), or an empty queue during normal operation.

The timeout is Host-implementation-defined. A Host should derive it from the bus bit rate ([BS-SPEC-100](/docs/technical-spec/) §6.4) and the maximum frame size (§5.1), plus a margin for card processing latency.

At the v1 bus bit rate of 1 Mbit/s, a maximum-size 293-byte frame occupies ~2.9 ms on the wire. A Host should allow a card a short bounded interval — on the order of 1 ms — to *begin* its response after a poll (covering driver turnaround and processing latency), and size its total receive window for a full maximum frame (~3 ms) plus margin.

## 5. Packet Framing

*Normative*

Every BBP packet uses the following frame. The field set and order are normative.

| Field | Description |
| ----- | ----------- |
| `SOF` | Start-of-frame delimiter |
| `SRC` | Source address — the sender's slot; Host-originated packets use a reserved Host source address |
| `DEST` | Destination address — `0` broadcast, `1`–`6` addressed slot (§6.2) |
| `SCHEMA_LEN` | Length of the schema name, in bytes |
| `SCHEMA` | Schema name — UTF-8 string (§7) |
| `PAYLOAD_LEN` | Length of the payload, in bytes |
| `PAYLOAD` | Schema-defined payload |
| `CRC` | Frame check sequence computed over the packet |

```
┌─────┬─────┬──────┬────────────┬────────┬─────────────┬─────────┬─────┐
│ SOF │ SRC │ DEST │ SCHEMA_LEN │ SCHEMA │ PAYLOAD_LEN │ PAYLOAD │ CRC │
└─────┴─────┴──────┴────────────┴────────┴─────────────┴─────────┴─────┘
                                 └ UTF-8 ┘              └ schema-defined ┘
```

`SCHEMA_LEN` and `PAYLOAD_LEN` make the schema name and payload variable-length. The field set and order above are normative; exact field widths, reserved address values, and the CRC algorithm are fixed in §5.1.

### 5.1 Reference wire format

*Normative*

| Field | Width | Value / range |
| ----- | ----- | ------------- |
| `SOF` | 1 byte | `0x7E`. Excluded from the CRC. |
| `SRC` | 1 byte | Sender address (§6): a slot `0x01`–`0x06`, or the Host `0x07`. |
| `DEST` | 1 byte | Destination (§6.2): `0x00` broadcast, `0x01`–`0x06` slot, `0x07` Host. |
| `SCHEMA_LEN` | 1 byte | Schema length, `0`–`31`. |
| `SCHEMA` | 0–31 bytes | UTF-8 schema name (§7). |
| `PAYLOAD_LEN` | 1 byte | Payload length, `0`–`255`. |
| `PAYLOAD` | 0–255 bytes | Schema-defined payload. |
| `CRC` | 2 bytes | Frame check (see below), big-endian. |

The CRC is **CRC-16/CCITT-FALSE** (polynomial `0x1021`, initial value `0xFFFF`, no input or output reflection, final XOR `0x0000`). It is computed over all bytes from `SRC` through `PAYLOAD` inclusive — the `SOF` delimiter is excluded — and transmitted most-significant byte first.

The maximum frame is **293 bytes**: `SOF` (1) + `SRC`/`DEST`/`SCHEMA_LEN` (3) + `SCHEMA` (31) + `PAYLOAD_LEN` (1) + `PAYLOAD` (255) + `CRC` (2).

## 6. Addressing

*Normative*

### 6.1 Address model

BBP addresses are physical slot numbers. Valid addresses are `1 2 3 4 5 6`. The SLOT analog signal (§3.3; [BS-SPEC-100](/docs/technical-spec/) §6.3) is the **sole source of BBP address assignment** in v1. No dynamic address assignment exists.

Two address values are reserved and are never assigned to a slot: `0` (broadcast, §6.2) and `7` (the Host source address). Addresses are one byte wide (§5.1).

### 6.2 Destination semantics

Every device on the RS-485 multidrop bus physically receives every packet. The `DEST` field determines which device acts on it.

| `DEST` | Meaning |
| ------ | ------- |
| `0` | Broadcast — every slot processes the packet |
| `1`–`6` | Addressed — only the named slot processes the packet |

Devices shall ignore the payload of addressed packets not directed at their slot.

### 6.3 Stable identity

Physical slot placement determines device identity. A module installed in Slot 2 shall always appear as Slot 2 after reboot.

## 7. Schema Layer

*Normative*

BBP transports **schema-defined payloads**. BBP itself defines framing, addressing, polling, and synchronization; payload interpretation is determined entirely by the schema name carried in the `SCHEMA` field.

- Schema names are UTF-8 strings.
- A schema name fully determines how its payload is encoded and interpreted.
- BBP does not inspect payload contents. Two endpoints interoperate by agreeing on a schema name.

Schema names are namespaced by convention (for example `controls.potbank`). The `bbp.` namespace is reserved for protocol-level messages (§8); module payload schemas shall not use the `bbp.` prefix.

## 8. Reserved Schemas

*Normative*

The `bbp.` namespace is reserved for built-in protocol messages. The following schemas are defined in BBP v1.

| Schema | Direction | Purpose |
| ------ | --------- | ------- |
| `bbp.identify` | Host → Card | Request module identification |
| `bbp.info` | Card → Host | Identification response: vendor, product, version, slot count |
| `bbp.poll` | Host → Card | Solicit a response from the addressed slot (zero or one packet) |
| `bbp.start` | Host → broadcast | Transport: start |
| `bbp.stop` | Host → broadcast | Transport: stop |
| `bbp.continue` | Host → broadcast | Transport: continue from the current position |
| `bbp.clock` | Host → broadcast | Clock tick (§10) |
| `bbp.error` | Card → Host | Report a protocol or application error |

Reserved-schema payloads are defined in §8.1 (`bbp.error`) and §9.3 (`bbp.info`). `bbp.poll`, `bbp.identify`, and the transport and clock schemas (`bbp.start`, `bbp.stop`, `bbp.continue`, `bbp.clock`) carry no payload.

Liveness needs no dedicated schema: a successful poll response proves a card is alive, and discovery (§9) re-establishes presence. `bbp.hello` and `bbp.heartbeat` were considered and dropped from v1 for this reason.

### 8.1 bbp.error payload

*Normative*

A `bbp.error` payload is at least one byte. Byte 0 is the **error class**; any remaining bytes are class-specific context.

| Error class | Value | Meaning | Context |
| ----------- | ----- | ------- | ------- |
| `RESERVED` | `0x00` | Reserved; shall not be sent. | — |
| `UNKNOWN_SCHEMA` | `0x01` | Schema name not recognized. | Echoed schema name |
| `BAD_PAYLOAD` | `0x02` | Payload failed schema validation. | — |
| `BUSY` | `0x03` | Card cannot service the request now. | — |
| `UNSUPPORTED` | `0x04` | Version or feature not supported. | — |
| `BAD_REQUEST` | `0x05` | Malformed or out-of-sequence request. | — |
| `INTERNAL` | `0x06` | Internal card error. | — |

A card reports an error by queuing a `bbp.error`; like every card transmission it is released only when the card is next polled (§4), ahead of queued events (§4.2).

## 9. Discovery

*Normative*

### 9.1 Discovery timing

Discovery shall occur during Host startup and during explicit Host-initiated rescans. BBP v1 does not support hot-plugging.

### 9.2 Discovery procedure

The Host shall query each slot individually, in slot order, using reserved schemas. The Host sends `bbp.identify` to prepare the module's response, then `bbp.poll` to retrieve it; the module answers with `bbp.info`. An empty slot returns nothing.

```
HOST → SLOT 1   bbp.identify
HOST → SLOT 1   bbp.poll
SLOT 1 → HOST   bbp.info     vendor=BUDDY product=POT-6 version=1.0 slots=1

HOST → SLOT 2   bbp.identify
HOST → SLOT 2   bbp.poll
SLOT 2 → HOST   bbp.info     vendor=BUDDY product=MIDI-IO version=1.0 slots=1

HOST → SLOT 3   bbp.identify
HOST → SLOT 3   bbp.poll
SLOT 3 → HOST   (no response — slot empty)

HOST → SLOT 4   bbp.identify
HOST → SLOT 4   bbp.poll
SLOT 4 → HOST   bbp.info     vendor=BUDDY product=OLED-128 version=1.0 slots=2

HOST → SLOT 5   bbp.identify
HOST → SLOT 5   bbp.poll
SLOT 5 → HOST   (no response — secondary slot of OLED-128, §9.4)

HOST → SLOT 6   bbp.identify
HOST → SLOT 6   bbp.poll
SLOT 6 → HOST   (no response — slot empty)
```

*FIG 3 — Discovery sequence, example bus population · identify + poll per slot, `bbp.info` reply · illustrative.*

### 9.3 Discovery response

Modules shall answer discovery with a `bbp.info` packet carrying the following fields.

| Field | Example |
| ----- | ------- |
| Vendor | BUDDY |
| Product | POT-6 |
| Version | 1.0 |
| Slot Count | 1 |

The payload is encoded as follows. Version is carried as separate major and minor bytes (`1.0` → major `1`, minor `0`). Vendor and Product are length-prefixed UTF-8 strings, each at most 24 bytes.

| Offset | Field | Width |
| ------ | ----- | ----- |
| 0 | Slot Count | 1 byte |
| 1 | Version major | 1 byte |
| 2 | Version minor | 1 byte |
| 3 | Vendor length (V) | 1 byte |
| 4 | Vendor | V bytes |
| 4 + V | Product length (P) | 1 byte |
| 5 + V | Product | P bytes |

### 9.4 Multi-slot modules

Modules occupying multiple slots shall designate a primary slot and report a Slot Count greater than 1. All communication shall occur through the primary slot. Secondary slots shall not respond to discovery requests.

*FIG 4 — Multi-slot module occupying slots 2–3 · communication via primary slot only. The OLED-128 (slots=2) occupies slots 2 and 3: primary = slot 2 (responds), secondary = slot 3 (silent). Schematic, not to scale.*

## 10. Synchronization

*Normative*

BBP synchronization uses **MIDI-style transport commands and clock events**, carried as reserved schemas (§8) and broadcast to all slots (`DEST = 0`):

| Schema | Role |
| ------ | ---- |
| `bbp.start` | Begin playback / sequencing from the start |
| `bbp.continue` | Resume from the current position |
| `bbp.stop` | Stop |
| `bbp.clock` | Advance one clock tick |

BBP does **not** distribute timestamps. Synchronization is event-based: modules advance on `bbp.clock` ticks and respond to transport commands, rather than scheduling against a shared wall clock. This is a deliberate protocol choice.

## 11. Philosophy

*Informative*

BBP v1 intentionally avoids a capability description framework.

Discovery answers **"what is connected?"** — it does not answer **"what does this module do?"** Module functionality is defined by module documentation, its payload schemas, and application firmware.

## 12. Product ID

*Normative*

Product identifiers are defined by module designers — for example `POT-6`, `MIDI-IO`, `OLED-128`, `CHAOS-LFO`. BBP does not maintain a centralized registry.

Product identifiers shall not be used as protocol addresses.

## 13. Future Scope

*Informative*

Potential future extensions include:

- Capability descriptors
- Firmware update support
- GUI tooling support
- Rich metadata
- Module self-description
- Card-to-card routing
- Advanced event subscriptions
- Timestamped synchronization

These features are explicitly outside the scope of BBP v1.

### Related documents

- [BS-SPEC-100](/docs/technical-spec/) — Mechanical + electrical standard
- [BS-DS-001](/design/) — Design system v0.1

## Revision History

| Rev | Date | Description |
| --- | ---- | ----------- |
| 1.0 DRAFT | JUN 2026 | Initial public draft. |
| 1.1 DRAFT | JUN 2026 | Reframed BBP as a host-polled transport carrying schema-defined packets: added Bus Access (polling model), Packet Framing, Schema Layer, Reserved Schemas, and Synchronization sections; added DEST broadcast/addressed semantics; discovery now expressed with `bbp.identify` / `bbp.poll` / `bbp.info`; SLOT named the sole source of address assignment. Added BBP scope exclusions (control/event only), the three-layer model, and a Module Requirements section (§2.4). |
| 1.2 DRAFT | JUN 2026 | Ratified the reference wire format (§5.1): field widths, reserved addresses (broadcast `0`, Host `7`), and CRC-16/CCITT-FALSE over `SRC`…`PAYLOAD`. Added response-queue ordering — protocol responses ahead of events (§4.2) — and the poll-response timeout (§4.3). Defined the `bbp.error` payload and error classes (§8.1). Removed `bbp.hello` and `bbp.heartbeat` from the reserved set (liveness is proven by a successful poll). |
| 1.3 DRAFT | JUN 2026 | Defined the `bbp.info` payload byte layout (§9.3): slot count, version major/minor, and length-prefixed vendor/product UTF-8 strings (≤24 bytes each). |
