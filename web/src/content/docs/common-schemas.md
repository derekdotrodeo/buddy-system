---
title: Common Schemas
description: BD-SPEC-300 defines a baseline catalog of recommended application payload schemas (controls, MIDI) so independent Buddy Cards interoperate.
order: 3
section: Specifications
---

The Buddy System Common Schemas define a baseline catalog of application payload schemas, so that independent Buddy Cards and hosts can interoperate without prior agreement.

**BD-SPEC-300** · Version 0.1 Draft · Jun 2026 · Requires [BD-SPEC-200](/docs/bus-protocol/)

## 1. Purpose

*Informative*

[BBP (BD-SPEC-200)](/docs/bus-protocol/) transports schema-named payloads but deliberately does not define what payloads mean (§7) — interpretation is carried by the schema name. This document defines a set of **common schemas** for frequently used controls and data so that modules from different designers work together out of the box.

Use of these schemas is **recommended, not required.** A module may use them, extend them, or define its own schemas entirely (BD-SPEC-200 §11). However, **when a module uses one of the schema names defined here, it shall use the payload layout defined here.**

## 2. Conventions

*Normative*

- Schema names are namespaced: `controls.*` for physical controls, `midi.*` for MIDI. The `bbp.*` namespace is reserved for the protocol (BD-SPEC-200 §8) and shall not be used for application schemas.
- Multi-byte integers are **big-endian**, matching BBP framing (BD-SPEC-200 §5.1).
- These are **event payloads**: a module reports one event per packet, drained by the Host's polls (BD-SPEC-200 §4).
- An **index** field identifies which instance of a control on a multi-control card, numbered from `0`.

## 3. controls.pot

*Normative*

Direction: **Card → Host.** An absolute analog control position — potentiometer, fader, or similar.

| Offset | Field | Width | Notes |
| ------ | ----- | ----- | ----- |
| 0 | index | u8 | which control, from `0` |
| 1 | value | u16 (BE) | `0`–`65535`, full-scale at the module's discretion |

Total: 3 bytes. A module with a lower-resolution converter shall left-justify its value into the 16-bit range so hosts can treat all pots uniformly.

## 4. controls.button

*Normative*

Direction: **Card → Host.** A button (or other two-state control) changed state.

| Offset | Field | Width | Notes |
| ------ | ----- | ----- | ----- |
| 0 | index | u8 | which button, from `0` |
| 1 | state | u8 | `0` released, `1` pressed |

Total: 2 bytes. Reported on change (debouncing is the module's responsibility).

## 5. controls.encoder

*Normative*

Direction: **Card → Host.** Relative movement of a rotary encoder since the last report.

| Offset | Field | Width | Notes |
| ------ | ----- | ----- | ----- |
| 0 | index | u8 | which encoder, from `0` |
| 1 | delta | i8 | signed detents; positive = clockwise |

Total: 2 bytes. Encoders report relative motion, not absolute position; the Host accumulates.

> `controls.button` and `controls.encoder` share a payload length. Endpoints shall dispatch on the schema **name**, not the payload length.

## 6. midi.message

*Normative*

Direction: **Card ↔ Host.** One MIDI message carried verbatim.

The payload is the raw MIDI message — a status byte followed by its data bytes (typically 1–3 bytes total). The payload length equals the message length.

Running status and System Exclusive (SysEx) are **out of scope for v0.1**: each packet shall carry exactly one complete channel-voice or system message with an explicit status byte.

## 7. Future Scope

*Informative*

Candidate additions for later revisions:

- `controls.led` / `controls.display` — Host → Card outputs (indicators, screens)
- Multi-channel / higher-resolution control banks in a single packet
- MIDI running status and SysEx chunking
- CV / gate / clock-divided trigger schemas

These are intentionally undefined in v0.1.

### Related documents

- [BD-SPEC-200](/docs/bus-protocol/) — Buddy Bus Protocol (transport + reserved schemas)
- [BD-SPEC-100](/docs/technical-spec/) — Mechanical + electrical standard

## Revision History

| Rev | Date | Description |
| --- | ---- | ----------- |
| 0.1 DRAFT | JUN 2026 | Initial catalog: `controls.pot`, `controls.button`, `controls.encoder`, `midi.message`. |
