---
title: Getting Started
description: Insert a Buddy Base, snap in your first Buddy Card, and patch.
order: 1
section: Guide
---

> Placeholder content. Real documentation will be ported from the design system and
> technical spec in a later pass.

The **Buddy Base** is the central control unit. Everything begins by seating it on your
breadboard and lining it up to the pitch — the 2.54mm hole spacing that defines the whole
system.

## 1. Seat the Buddy Base

Place the Buddy Base so its pins straddle the center channel of the breadboard. Press
gently until it sits flush.

## 2. Snap in a Buddy Card

Each card belongs to a color-coded family:

- **Control** (orange) — pots, switches
- **Sound** (yellow) — amps, filters, synth voices
- **Data** (green) — MIDI and data I/O
- **Logic** (blue) — clocks, sequencers

```text
BD-CTL-01  Pots Buddy
BD-LGC-01  Clock Buddy
BD-SND-02  Filter Buddy
```

## 3. Patch with jumpers

Red is always `+V`. Blue is always `GND`. Everything else is signal — wire it however
your patch demands. Nothing you do here is permanent.

Next: see [the feature overview](/features/) for what each family can do.
