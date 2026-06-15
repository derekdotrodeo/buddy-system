---
title: Introducing Buddy System
titleHtml: 'Introducing <em>Buddy</em> System'
description: 'Buddy System is a modular prototyping platform for building synthesizers — a breadboard, plug-in cards, and a two-wire bus in one. Less wiring, less debugging, more sounds.'
deck: "Building synths on breadboards, I got tired of potentiometers that wouldn't stay put. Or of realizing, two hours into a bug, that I'd pushed a jumper into the wrong tiny square in a jungle of wires. Or looking up how to wire a MIDI circuit for the 10th time. So I built a tool. It worked pretty well — and now I'm making it better, because I think it can work for you too."
pubDate: 2026-06-15
author: Derek Wheelden
authorInitials: DW
tags: [introduction, buddy-system, diy-synth, open-hardware, breadboard]
family: dat
kicker: 'ANNOUNCEMENT · BD-LOG-001 · THE FIRST POST'
crumb: ANNOUNCEMENTS
famStrip: true
postType: "NEWS"
hero:
    src: /blog/hardware-render.png
    alt: 'The Buddy System — a base unit carrying Knob and MIDI Buddies, a switch bank, two open module bays and a 60-row breadboard with a host microcontroller mid-board.'
    caption: 'FIG 0 · The Buddy System — Base, Knob & MIDI Buddies, a switch bank and two open bays over a 60-row breadboard.'
    ref: 'BD-000 · REV A'
toc:
    - { id: s1, label: The elevator pitch }
    - { id: s2, label: Why I'm building it }
    - { id: s3, label: Is this for you? }
    - { id: s4, label: How it works }
    - { id: s5, label: Bring your own brain }
    - { id: s6, label: Design principles }
    - { id: s7, label: What I've built so far }
    - { id: s8, label: Follow along }
# next: { href: '#', title: Anatomy of a beep }
# related:
#     - { href: '#', cat: 'SND · BUILD LOG', title: Anatomy of a beep, meta: '9 MIN · 4 JUMPERS · ★ BEGINNER', family: snd }
#     - { href: /docs/technical-spec/, cat: 'UTY · DOCUMENTATION', title: The Buddy Base spec, meta: 'BD-SPEC-100 · REFERENCE', family: uty }
#     - { href: /docs/bus-protocol/, cat: 'DAT · DOCUMENTATION', title: The Buddy Bus Protocol, meta: 'BD-SPEC-200 · REFERENCE', family: dat }
draft: false
---

<h2 id="s1"><span class="snum">§1</span> The elevator pitch</h2>

<p>Buddy System is a modular prototyping platform for building synthesizers. It combines a breadboard, plug-in modules, and a simple communication bus into a single system designed to reduce wiring, simplify setup, and make synth development more approachable. Whether you're generating your first sine wave or designing your own instrument, Buddy System helps you spend more time making sounds and less time debugging hardware. Turning electricity into music is hard, but it's easier with a Buddy.</p>

<div class="pullquote"><p>Turning electricity into music is hard. It's easier with a Buddy.</p><cite>— The whole idea, in one line</cite></div>

<h2 id="s2"><span class="snum">§2</span> Why I'm building it</h2>

<p>I've been tinkering with electronics most of my life, and I've been a software engineer for the past 15 years. I started building synths a few years ago, learning the basics on breadboards and protoboards. As a programmer I gravitated toward digital synths, learning about oscillators, envelopes, filters and how to implement those in C. Making that first sine wave with just a few lines of code was <em>so</em> satisfying.</p>

<aside class="marginnote"><span class="mn-tag">The frustration</span>Loose connections. Pots that pop out the moment you touch them. A project that worked on Tuesday and mysteriously doesn't on Thursday. None of that is the fun part.</aside>

<p>The hardware side felt a little more frustrating. Fighting with potentiometers that didn't really want to stay on the breadboard, hours of debugging a loose connection, coming back to a project after a couple days that just mysteriously doesn't work anymore. I wanted to focus on creating sound, not fiddling with wires.</p>

<p>So I thought I could make a tool that would make my life easier, and learned along the way that I could make a tool that would make DIY synths more accessible and approachable for everyone else too.</p>

<h2 id="s3"><span class="snum">§3</span> Is this for you?</h2>

<p>If you're interested in turning electricity into music, Buddy System is for you at any skill level. Maybe you can write a little code, but you're intimidated by the electronics. That's fine, Buddy System takes most of the hardware set up out of the equation. Just plug in the Buddy Cards you need, and start coding. Oh, you're actually an audio synthesis pro who's sick of dealing with fragile breadboards? Buddy Cards don't fall out when you touch them, you're gonna love this thing.</p>

<div class="callout tip"><span class="co-chip">EITHER WAY</span><div class="co-body"><p><strong>Coder who's wary of hardware?</strong> The cards handle the circuitry — you just read and write pins. <strong>Hardware veteran tired of flaky breadboards?</strong> Keyed connectors, solid seating, no mystery faults. Same system, both problems solved.</p></div></div>

<h2 id="s4"><span class="snum">§4</span> How it works</h2>

<p>At its core, Buddy System is three parts: the <strong>Buddy Base</strong>, the <strong>Buddy Cards</strong>, and the <strong>Buddy Bus Protocol</strong>. Plug a card in anywhere and it's connected everywhere.</p>

<div class="modules"><div class="module" style="--fam: var(--fam-uty);"><div class="m-rail"><span class="m-num">01</span><span class="m-ref">BD-000</span></div><div class="m-body"><h3>Buddy Base <span class="chip" style="--fam: var(--fam-uty);">UTY</span></h3><p>The hub — the nervous system of the whole thing. It connects every piece physically and electrically, provides power to your modules and to the breadboard, and routes the bus everywhere. As a bonus, Base gives you <strong>10 LEDs</strong> you can use for anything.</p></div></div><div class="module" style="--fam: var(--fam-ctl);"><div class="m-rail"><span class="m-num">02</span><span class="m-ref">BD-CTL-01 +</span></div><div class="m-body"><h3>Buddy Cards <span class="chip" style="--fam: var(--fam-ctl);">CTL</span></h3><p>Your physical interface — small modular units that plug directly into the Base. A card can be anything: a bank of potentiometers, a MIDI interface, an OLED display, whatever you dream up. Wire them to the breadboard with jumpers, or let them talk digitally over the bus using just <strong>2 jumpers for all 6 cards</strong>.</p></div></div><div class="module" style="--fam: var(--fam-dat);"><div class="m-rail"><span class="m-num">03</span><span class="m-ref">BD-SPEC-200</span></div><div class="m-body"><h3>Buddy Bus Protocol <span class="chip" style="--fam: var(--fam-dat);">DAT</span></h3><p>BBP is the communication standard. Modules talk over a shared two-wire bus, slashing the wiring between your controls and your synth. Cards <strong>self-identify</strong>, so software-to-hardware mapping stays consistent and predictable. BBP carries bits, strings, samples — even complex objects, up to <code class="ic">255</code> bytes per packet.</p></div></div></div>

<h2 id="s5"><span class="snum">§5</span> Bring your own brain</h2>

<p>This is where you come in. Buddy System doesn't care what microcontroller you use. Daisy Seed, Teensy, Arduino, Raspberry Pi Pico, or something else entirely — <strong>if it can talk to BBP, it can be part of the system.</strong></p>

<div class="brains"><span class="brain"><i></i>Daisy Seed</span><span class="brain"><i></i>Teensy</span><span class="brain"><i></i>Arduino</span><span class="brain"><i></i>RP2040 · Pico</span><span class="brain any"><i></i>…anything that speaks BBP</span></div>

<p>This is where you get to express your creativity. Maybe you're building a simple Arduino synth to learn the basics. Maybe you're squeezing every last cycle out of a high-performance DSP platform. Maybe you're not writing code at all and just want a convenient way to connect controls and interfaces to an analog circuit. Buddy System is designed to support all of those.</p>

<div class="callout note"><span class="co-chip">NOTE</span><div class="co-body"><p>The goal isn't to lock you into an ecosystem. The goal is a solid foundation, so you can focus on building the instrument <strong>you</strong> want to build.</p></div></div>

<h2 id="s6"><span class="snum">§6</span> Design principles</h2>

<p>Four ideas guide every decision on this project. Color is wiring here, so each one gets its own family hue.</p>

<div class="principles"><div class="principle" style="--fam: var(--fam-dat);"><span class="p-key"></span><span class="p-idx">PRINCIPLE 01</span><h4>Simplicity</h4><p>Reduce friction. Cut complexity and get out of the way of your creativity. Plug and play, simple protocols, reliable hardware.</p></div><div class="principle" style="--fam: var(--fam-uty);"><span class="p-key"></span><span class="p-idx">PRINCIPLE 02</span><h4>Openness</h4><p>Every part of Buddy System is open source from schematics to firmware, all available to and free to use. Fork out libraries. Take our schematics and design your own PCBs.</p></div><div class="principle" style="--fam: var(--fam-ctl);"><span class="p-key"></span><span class="p-idx">PRINCIPLE 03</span><h4>Modularity</h4><p>Flexible by design. Simple modules for the basics, but unlimited potential — plug-and-play cards, breakout headers, and room for a large breadboard. What can't you build?</p></div><div class="principle" style="--fam: var(--fam-lgc);"><span class="p-key"></span><span class="p-idx">PRINCIPLE 04</span><h4>Education</h4><p>More people should be able to build cool synths. By removing complicated circuitry, fragile breadboards, and hardware debugging, we lower the barrier to entry.</p></div></div>

<h2 id="s7"><span class="snum">§7</span> What I've built so far</h2>

<p>Last year I built the first Buddy System prototype. With a few basic modules, it taught me a lot — and more importantly, it convinced me that I really wanted this thing to exist.</p>

<figure class="fig">
    <div class="holder">
        <span class="keytab" style="--fam: var(--fam-uty);"></span>
        <div class="ph tall">
            <span class="ph-dim"><img src="/blog/buddy-system-proto.jpeg" /></span>
        </div>
    </div>
    <figcaption>
        <span class="fnum">FIG 1</span>
        Where it started — the original prototype that proved the idea was worth chasing
    </figcaption>
</figure>

<p>This year I'm designing from scratch, using everything the prototype taught me. The <strong>Buddy Base</strong> is designed and ready for production. So is the first card — <strong>Knob Buddy</strong>, a bank of six potentiometers. Right now I'm deep in the Buddy Bus Protocol: a platform-agnostic C library with thin, hardware-specific abstraction layers on top, so the same code runs whether your brain is a Pico or a Daisy.</p>

<figure class="fig">
    <div class="holder">
        <span class="keytab" style="--fam: var(--fam-uty);"></span>
        <div class="ph">
            <span class="ph-dim"><img src="/blog/buddy-base-render.png" /></span>
        </div>
    </div>
    <figcaption>
        <span class="fnum">FIG 2</span>
        Where it's going — the next iteration of the Buddy Base
    </figcaption>
</figure>

<div class="roadmap"><div class="rm-cap">Build status · the road so far, and the road ahead</div><div class="rm-item done"><span class="rm-stage">2025</span><span class="rm-what">First prototype <span class="rm-sub">A few basic modules. Proved the concept — and the appetite.</span></span><span class="rm-badge">Shipped</span></div><div class="rm-item done"><span class="rm-stage">2026</span><span class="rm-what">Buddy Base <span class="rm-sub">Redesigned from scratch. Power, bus, breadboard, 10 LEDs.</span></span><span class="rm-badge">Ready for production</span></div><div class="rm-item done"><span class="rm-stage">2026</span><span class="rm-what">Knob Buddy <span class="rm-sub">The first card — a bank of six potentiometers.</span></span><span class="rm-badge">Ready for production</span></div><div class="rm-item now"><span class="rm-stage">In progress</span><span class="rm-what">Buddy Bus Protocol <span class="rm-sub">Platform-agnostic C library plus hardware abstraction layers.</span></span><span class="rm-badge">Writing firmware</span></div><div class="rm-item next"><span class="rm-stage">Next</span><span class="rm-what">MIDI &amp; Line-Level Cards <span class="rm-sub">The proof BBP can carry MIDI and audio samples cleanly.</span></span><span class="rm-badge">On the bench</span></div></div>

<h2 id="s8"><span class="snum">§8</span> Follow along</h2>

<p>From here, I'll be documenting the design process — sharing the successes and the blue smoke, and posting plenty of demos along the way. We'll talk hardware design, get into the weeds of digital communication, and share in the bleeps and the bloops.</p>

<p>Follow along, sign up for the newsletter, or just lurk. It's up to you, Buddy.</p>
