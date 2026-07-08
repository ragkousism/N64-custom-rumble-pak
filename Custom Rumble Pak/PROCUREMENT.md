# Procurement Checklist

Parts and tools to gather for the RP2040 rumble-pak conversion. See
[`DESIGN.md`](DESIGN.md) for how each piece is used.

Legend: `[x]` have · `[~]` likely have / to confirm · `[ ]` need to source.

## A. Core parts

- [x] Broken aftermarket Memory Pak (donor — for its edge connector + shell)
- [x] RP2040 board — **YD-RP2040 clone (chosen)**: genuine RP2040 die, ME6217 LDO
      (hence the 3-cell battery below), verified running the debug firmware. A second
      board is inbound — keep it as a bench debug/probe board (or swap it in if it
      turns out to be a genuine Pico and 2 cells fit the shell better).
- [~] DC vibration/rumble motor — salvaged from a PlayStation (DualShock) controller,
      ~3 V ERM. Confirm its stall current (sets FET + fuse). Over-driven by the 3-cell
      pack → use the firmware PWM duty cap (see DESIGN.md).
- [ ] **3× AAA NiMH cells** (low-self-discharge / Eneloop-type) — ~3.6 V pack, sized
      for the YD-RP2040's LDO (needs ≳3.4 V in). Charged externally (no onboard
      charger).
- [ ] 3-cell AAA battery holder + small on/off switch (check it fits the pak shell)
- [ ] Bulk cap for VSYS hold-up — 100–470 µF (motor-inrush brown-out protection)

## B. Motor driver stage

- [ ] Logic-level N-MOSFET — **AO3400A** (or 2N7002 only for a tiny ~200 mA motor) ×1
- [ ] Flyback Schottky diode — **SS14** or 1N5819 ×1
- [ ] Gate resistor — 100 Ω ×1
- [ ] Gate pulldown resistor — 100 kΩ (keeps motor off at boot) ×1
- [ ] Resettable **PTC fuse** — just above motor stall current ×1
- [ ] Bulk cap — 47–100 µF electrolytic (motor) ×1
- [ ] Ceramic 100 nF — motor terminals + VSYS decoupling ×3–4
- [ ] 10 µF cap — VSYS ×1

## C. Bus interconnect

- [ ] Series bus resistors *(optional protection)* — 33–100 Ω, ×~20 (a resistor
      array/network saves space)
- [ ] Fine wire — 30 AWG wire-wrap or enameled magnet wire, for edge-connector pads
- [ ] Small perfboard / proto area to mount the Pico + driver (or dead-bug it)

## D. Tools (strip + assembly)

- [ ] Hot-air rework station **or** ChipQuik/low-melt alloy — to lift the SOP-32 SRAM,
      HC gate, U3, coin cell, and button off the donor
- [ ] Fine-tip soldering iron + thin solder
- [ ] Flux, desoldering braid
- [ ] Tweezers
- [ ] Magnification (loupe / USB microscope) — fine-pitch pads
- [ ] Multimeter — continuity-check edge-connector pin → pad after stripping
- [ ] Kapton tape, heatshrink

## E. Test / firmware phase (not needed to start building)

- [x] USB-C cable — flash the YD-RP2040 (done once already)
- [ ] Logic analyzer (cheap 8-ch is fine) or scope — observe the bus; confirm
      probe/motor addresses and timing
- [ ] N64 + controller + a rumble game (or a libdragon rumble test) for end-to-end test

## Decisions that affect the buy

1. **Motor stall current** → sets the PTC fuse rating (and confirms the FET headroom).
2. **Optional bus series resistors** — include for protection, or skip for fewer parts
   (acceptable at these bus speeds).
