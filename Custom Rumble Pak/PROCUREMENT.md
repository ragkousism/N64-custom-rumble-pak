# Procurement Checklist

Parts and tools to gather for the RP2040 rumble-pak conversion. See
[`DESIGN.md`](DESIGN.md) for how each piece is used.

Legend: `[x]` have · `[~]` likely have / to confirm · `[ ]` need to source.

## A. Core parts

- [x] Broken aftermarket Memory Pak (donor — for its edge connector + shell)
- [~] Raspberry Pi Pico (RP2040) — plain Pico is fine (3.3 V, has PIO)
- [~] DC vibration/rumble motor — salvaged from a PlayStation (DualShock) controller,
      ~3 V ERM. Confirm its stall current (sets FET + fuse).
- [ ] **2× AAA NiMH cells** (low-self-discharge / Eneloop-type) — ~2.4 V pack. Charged
      externally (no onboard charger).
- [ ] 2-cell battery holder + small on/off switch
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

- [ ] micro-USB cable — flash the Pico
- [ ] Logic analyzer (cheap 8-ch is fine) or scope — observe the bus; confirm
      probe/motor addresses and timing
- [ ] N64 + controller + a rumble game (or a libdragon rumble test) for end-to-end test

## Decisions that affect the buy

1. **Motor stall current** → sets the PTC fuse rating (and confirms the FET headroom).
2. **Optional bus series resistors** — include for protection, or skip for fewer parts
   (acceptable at these bus speeds).
