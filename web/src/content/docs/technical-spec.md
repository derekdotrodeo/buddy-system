---
title: Mechanical & Electrical Standard
description: BS-SPEC-100 defines the mechanical and electrical standard for the Buddy System — a Eurorack-compatible Buddy Base and interchangeable Buddy Cards.
order: 1
section: Specifications
---

This document defines the mechanical and electrical standards for the Buddy System platform.

**BS-SPEC-100** · Normative · Version 1.2 Draft · Jun 2026 · Units: Millimeters

## 1. Purpose

*Normative*

This document defines the mechanical and electrical standards for the Buddy System platform.

Buddy System is a Eurorack-compatible electronics and synthesizer prototyping platform consisting of a central Buddy Base and interchangeable Buddy Cards. The platform is designed to provide reusable infrastructure for digital synthesizer development, interactive electronics, and educational experimentation.

Unless otherwise specified, all dimensions are in millimeters.

## 2. Terminology

*Normative*

**Buddy Base** — The primary Eurorack-compatible carrier board containing:

- Buddy Card slots
- Power distribution
- Buddy Bus infrastructure
- Prototyping breadboard
- Utility circuitry

**Buddy Card** — A plug-in expansion card providing reusable functionality such as:

- Controls
- Indicators
- MIDI interfaces
- Audio interfaces
- Displays
- Communications
- Utility circuits

**Buddy Bus** — The shared electrical and mechanical interconnect system used between the Buddy Base and Buddy Cards.

## 3. Mechanical

*Normative*

### 3.1 Buddy Base dimensions

The Buddy Base shall conform to standard Eurorack mechanical dimensions.

| Parameter | Value |
| --- | --- |
| Width | 47 HP (238.76 mm) |
| Height | 3U (128.5 mm nominal) |
| PCB thickness | 2.0 mm |

*FIG 1 — Buddy Base, front view · functional regions per §3.2–3.3 · scale 2.8 px/mm. The base outline (238.76 × 128.5 mm) shows an upper region with six dashed card slots numbered 1–6 on 39.0 TYP pitch, and a lower region containing the 830-point solderless breadboard, a power-in area (USB-C / HDR), a status area, and a bus breakout · test points column. Overall dimensions 238.76 · 47 HP wide by 128.5 · 3U tall.*

### 3.2 Functional layout

The Buddy Base is divided into two primary regions. The **upper region** is reserved for Buddy Card slots. The **lower region** is reserved for the 830-point solderless breadboard, power distribution circuitry, status indicators, bus breakout headers, test points, and utility functions.

The lower region shall provide sufficient space for a standard 830-point solderless breadboard.

### 3.3 Slot arrangement

The Buddy Base shall provide six Buddy Card slots, numbered left to right `1 2 3 4 5 6`. Slot pitch shall be 39 mm center-to-center.

### 3.4 Buddy Card dimensions

| Parameter | Value |
| --- | --- |
| Width (single slot) | 38.0 mm ±0.2 |
| Height | 64.0 mm |
| PCB thickness | 1.6 mm |
| Max component protrusion below PCB | 8.0 mm |

*FIG 2 — Buddy Card, front view · connector centers per §4.2 · origin at lower-left per §3.6 · scale 4.2 px/mm. The card outline (38 × 64 mm) shows a 1.5 mm edge keepout (§3.7), an upper 1×6 connector at 2.54 pitch centered at (19, 60) and a lower 1×6 connector at 2.54 pitch centered at (19, 4), with pin 1 leftmost. Connector spacing is 56.0 mm; card height 64.0 mm; card width 38.0 ±0.2 mm; origin (0,0) at lower-left with X and Y axes.*

### 3.5 Inter-card spacing

Buddy Cards are installed on a 39 mm pitch. This results in a nominal 1 mm gap between adjacent cards.

### 3.6 Coordinate system

Buddy Card layouts shall use an origin at the lower-left corner, with units in millimeters. See FIG 2.

### 3.7 Edge keepout

A minimum keepout of 1.5 mm shall be maintained from all card edges for component placement.

### 3.8 Component clearance

Components may extend any distance above the PCB provided they do not interfere with adjacent cards or exceed the Buddy Base mechanical envelope. Components on the rear side of the PCB shall not extend more than 8 mm below the PCB surface.

*FIG 3 — Buddy Card, side view · rear clearance limit per §3.8 · not to scale. Front components have no limit within the envelope (§3.8); the PCB is 1.6 thk; rear components extend no more than 8.0 MAX below the PCB surface.*

### 3.9 Multi-slot Buddy Cards

Multi-slot Buddy Cards are permitted. A multi-slot card occupies two or more adjacent slot positions.

Card width shall be:

```
width = (39 × occupied_slots) − 1  mm
```

| Slots | Width |
| --- | --- |
| 1 | 38 mm |
| 2 | 77 mm |
| 3 | 116 mm |

A single-slot card therefore measures 38.0 mm, consistent with §3.4. Each occupied slot shall populate both Buddy Bus connectors (see §4.3).

### 3.10 Identification area

Each Buddy Card shall reserve a minimum 10 mm × 10 mm area in the upper-left region of the PCB for identification markings.

The following markings are **required**:

- Module name
- Revision

The following markings are **optional**:

- Designer
- Manufacturer
- URL
- QR code

## 4. Bus · Mechanical

*Normative*

### 4.1 Connector type

The Buddy Bus consists of two 1×6 connectors using 2.54 mm (0.1 inch) pitch. Connectors face the front of the system and provide both electrical interconnection and mechanical retention.

| Location | Connector |
| --- | --- |
| Buddy Base | FEMALE |
| Buddy Card | MALE |

### 4.2 Connector placement

Connector centers shall be located as follows, with 56 mm center-to-center spacing. See FIG 2.

| Connector | X | Y |
| --- | --- | --- |
| Lower | 19 | 4 |
| Upper | 19 | 60 |

### 4.3 Connectors per slot

Two Buddy Bus connectors are required for every occupied slot. A single-slot Buddy Card populates one upper and one lower connector. A multi-slot Buddy Card shall populate both the upper and lower Buddy Bus connectors at each slot position it occupies. For example, a three-slot card requires six Buddy Bus connectors — an upper and lower pair at each of its three slots.

## 5. Bus · Electrical

*Normative*

### 5.1 Pinout

When viewed from the front of a Buddy Card, pin 1 shall be the leftmost pin.

*FIG 4 — Buddy Bus pinout · viewed from front of card · pin 1 leftmost · both connectors identical.*

| Pin | Signal | Function |
| --- | --- | --- |
| 1 | SLOT | SLOT IDENTIFICATION |
| 2 | BUS A | RS-485 A |
| 3 | BUS B | RS-485 B |
| 4 | 3V3 | +3.3 V |
| 5 | 5V | +5 V |
| 6 | GND | GROUND |

### 5.2 Power rails

| Rail | Nominal | Tolerance | Description |
| --- | --- | --- | --- |
| 3V3 | 3.3 V | ±5% | Regulated 3.3 V supply, generated locally by the Buddy Base |
| 5V | 5.0 V | ±5% | Main system supply |
| GND | 0 V | — | Common ground |

Buddy Cards may generate additional voltages locally as required. Additional voltages are outside the scope of this specification.

Buddy Cards shall not back-power the 3V3 or 5V rails. These rails are driven exclusively by the Buddy Base.

### 5.3 Power sources

The Buddy Base shall accept a 5 V input source. Supported input methods include USB-C and a 2-pin 0.1 inch header. The Buddy Base shall generate the system 3.3 V rail locally.

### 5.4 Grounding

All Buddy Cards share a common ground plane through the Buddy Bus.

## 6. Bus · Comms

*Normative*

### 6.1 Physical layer

BUS A and BUS B form a shared multidrop RS-485 communication bus. All Buddy Card slots are connected in parallel.

*FIG 5 — RS-485 multidrop topology · termination at base only per §6.2 · schematic, not to scale. Slots 1–6 each tap BUS A and BUS B in parallel; both bus lines are terminated at the base only (TERM (BASE) at each end).*

### 6.2 Bus termination

Bus termination shall be provided by the Buddy Base. Buddy Cards shall not provide bus termination.

Buddy Cards shall not provide RS-485 bias resistors. Bus conditioning — both termination and biasing — is owned exclusively by the Buddy Base.

### 6.3 SLOT signal

SLOT provides a unique analog voltage, referenced to the 3V3 rail, generated by the Buddy Base for each slot position. BBP-capable modules shall measure SLOT using an ADC and map the result to a slot number.

Each slot's nominal SLOT voltage shall be as follows:

| Slot | SLOT voltage (nominal) | Fraction of 3V3 |
| --- | --- | --- |
| 1 | 0.30 V | 0.091 |
| 2 | 0.59 V | 0.180 |
| 3 | 1.06 V | 0.320 |
| 4 | 1.65 V | 0.500 |
| 5 | 2.27 V | 0.688 |
| 6 | 2.72 V | 0.825 |

Modules shall determine slot number by thresholding the measured voltage at the midpoints between adjacent nominal levels. The nominal voltages above are normative; the resistor-divider network on the Buddy Base that produces them is an implementation detail and is outside the scope of this specification.

> **Note (informative):** These voltages are produced by the current Buddy Base resistor dividers — a fixed 100k high-side resistor from 3V3 and a per-slot low-side resistor to ground: Slot 1 = 10k, Slot 2 = 22k, Slot 3 = 47k, Slot 4 = 100k, Slot 5 = 220k, Slot 6 = 470k. The divider network is a Base implementation detail; the nominal voltages above are the normative interface.

The SLOT signal is foundational to the addressing and discovery model defined in [BS-SPEC-200 · Buddy Bus Protocol](/docs/bus-protocol/).

### 6.4 Bus signaling

The Buddy Bus operates as a 2-wire half-duplex RS-485 link. At most one device drives the bus at any instant; the host-controlled polling model ([BS-SPEC-200](/docs/bus-protocol/) §4) guarantees this, so the physical layer requires no collision arbitration.

| Parameter | Value |
| --- | --- |
| Bit rate | 1 Mbit/s |
| Character format | 8 data bits, no parity, 1 stop bit (8N1) |
| Bit order | LSB first |
| Idle line state | Mark (recessive) |
| Duplex | Half-duplex, 2-wire (BUS A / BUS B) |

The bit rate is fixed for BBP v1; dynamic rate negotiation is not supported. At 1 Mbit/s a maximum-size BBP frame (293 bytes; [BS-SPEC-200](/docs/bus-protocol/) §5.1) occupies approximately 2.9 ms on the wire.

### 6.5 Protocol layer

Protocol behavior — addressing, discovery, synchronization, source identification, packet framing, and the polling model — is defined by [BS-SPEC-200 · Buddy Bus Protocol](/docs/bus-protocol/). This specification defines only the physical and electrical layer.

## 7. Protection

*Normative*

The Buddy Base shall provide over-current protection for the standard power rails. Implementation details are outside the scope of this specification.

## 8. Hot-Plugging

*Normative*

Hot-plugging of Buddy Cards is not supported.

> **Caution:** Buddy Cards shall only be inserted or removed while the Buddy Base is powered off.

## 9. Visual Design

*Informative*

This specification does not impose requirements regarding silkscreen placement, logos, labeling conventions, color schemes, or aesthetic design. Visual consistency is encouraged but not required for compliance.

> **See:** The recommended visual language — faceplate anatomy, family color coding, and silkscreen conventions — is defined separately in the [Buddy System Design System v0.1](/design/). Compliance with this specification is mechanical and electrical only.

## 10. Future

*Informative*

The Buddy Bus protocol layer — message format, addressing, discovery, synchronization, and source identification — is now defined in [BS-SPEC-200 · Buddy Bus Protocol](/docs/bus-protocol/). Future revisions of this specification may define: current-limit thresholds, hot-plug support, and additional power capabilities. These topics remain intentionally undefined in this version.

## Revision History

| Rev | Date | Description |
| --- | --- | --- |
| 1.0 DRAFT | JUN 2026 | Initial public draft. |
| 1.1 DRAFT | JUN 2026 | Pin 1 redefined AUX → SLOT for slot identification; multi-slot cards and per-slot connector requirement added; component identification area added; back-powering and RS-485 biasing prohibited; protocol layer delegated to BS-SPEC-200. Power-rail ±5% tolerances, multi-slot width examples, and a normative SLOT voltage table (slots 1–6) added. |
| 1.2 DRAFT | JUN 2026 | Added Bus signaling (§6.4): bit rate 1 Mbit/s, 8N1, LSB-first, idle mark, 2-wire half-duplex — the physical-layer parameters BBP (BS-SPEC-200 §4.3) depends on. Protocol-layer subsection renumbered §6.4 → §6.5. |
