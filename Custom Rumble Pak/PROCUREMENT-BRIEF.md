# Sourcing Brief — N64 Custom Rumble Pak (standalone)

> Self-contained context dump for discussing parts sourcing (e.g. with an assistant on
> the go). Everything needed to evaluate parts/substitutes is in this one file. The
> authoritative docs are `DESIGN.md` / `PROCUREMENT.md` / `WIRING.md` in this folder —
> if this file disagrees with them, they win.

## Project in three sentences

A broken aftermarket N64 Controller (Memory) Pak is stripped down to its bare card-edge
connector and rebuilt as a **Rumble Pak**: an RP2040 board emulates the rumble pak
protocol on the controller's 3.3 V parallel accessory bus and switches a salvaged
DualShock vibration motor via a MOSFET. The electronics are powered **only from an
onboard battery — never from the console** (hard constraint: drawing power from the
N64's accessory 3.3 V rail can damage the console). Firmware is written and bench-
verified; what remains is buying the parts below and assembling.

## Fixed decisions (don't re-litigate while shopping)

- **MCU board: YD-RP2040 clone (in hand, working).** Its regulator is an **ME6217C33
  LDO** (not the genuine Pico's buck-boost), so it needs **≥ 3.4 V in** to make 3.3 V.
- **Battery: 3× AAA NiMH** (~3.6 V nominal, ~4.3 V fresh, ~3.1 V = empty). Charged in an
  external charger; no charging circuit in the pak.
- **Motor: salvaged PlayStation DualShock ERM motor, ~3 V rated.** The 3.6 V pack
  over-drives it; firmware PWM caps the duty (~75–80 %) to compensate. No motor to buy.
- Everything must eventually fit inside a Controller-Pak-sized shell
  (roughly 50 × 47 × 20 mm internal; a 3D-printed shell comes later).

## Shopping list

### Power
| Item | Spec | Qty | Notes |
|---|---|---|---|
| AAA NiMH cells | low self-discharge (Eneloop-type), ~800 mAh | 3 (+3 spare set nice) | pack must hold ≥ 3.4 V under load |
| AAA holder, 3-cell | flat / side-by-side preferred | 1 | must fit the shell — thin matters more than short |
| Slide switch | SPST, ≥ 1 A DC | 1 | battery master switch; also prevents back-powering |
| NiMH charger | any decent AAA charger | 0–1 | only if not already owned |

### Motor driver (one jellybean-parts order)
| Item | Spec | Qty | Notes |
|---|---|---|---|
| N-MOSFET | **AO3400A** (SOT-23, logic-level: fully on at 2.5–3.3 V gate, ~5.7 A) | 2 (1 + spare) | substitutes OK if: Vgs(th) ≤ 1.5 V, Rds(on) spec'd at 2.5 V gate, Id ≥ 2× motor stall current |
| Schottky diode | **SS14** or **1N5819** (1 A, 40 V) | 2 | flyback across the motor; any 1 A+ Schottky works |
| Resistor 100 Ω | any, 1/8 W+ | 2 | FET gate series |
| Resistor 100 kΩ | any, 1/8 W+ | 2 | gate pulldown (motor off at boot) |
| PTC resettable fuse | hold current just above motor **stall** current | 2 | ⚠️ rating unknown until the motor's stall current is measured — buy last, or buy a small assortment (0.5 A / 1 A / 1.5 A hold) |
| Electrolytic cap | 100–470 µF, ≥ 6.3 V | 1 | bulk on battery/Vin node (brown-out ride-through) |
| Electrolytic cap | 47–100 µF, ≥ 6.3 V | 1 | motor supply local bulk |
| Ceramic cap 100 nF | X7R, any voltage | 4 | motor terminals + decoupling |
| Ceramic/tantalum 10 µF | ≥ 6.3 V | 1 | Vin decoupling |

### Interconnect / assembly
| Item | Spec | Qty | Notes |
|---|---|---|---|
| Fine wire | 30 AWG wire-wrap (Kynar) or enameled magnet wire | 1 roll | 15 signal wires solder to small edge-connector pads |
| Perfboard | small piece, 2.54 mm | 1 | mounts RP2040 board + FET stage |
| Series resistors *(optional)* | 33–100 Ω | ~20 or 1 resistor network | bus protection; skippable at these speeds |

### Tools (buy only if missing)
- Hot-air rework station **or** ChipQuik low-melt alloy — to lift a SOP-32 SRAM and
  small glue logic off the donor board without ripping pads
- Fine-tip soldering iron, thin solder, flux, desoldering braid, tweezers
- Multimeter (mandatory: continuity mapping + motor stall-current measurement)
- Loupe or USB microscope; Kapton tape; heatshrink
- Cheap 8-channel logic analyzer (fx2/"24 MHz 8ch" type) — for bus timing validation;
  strongly recommended, not strictly blocking

## The one measurement gating a purchase

**Motor stall current** (multimeter in series, motor held stalled, at ~3 V): sets the
**PTC fuse** hold rating (just above stall) and confirms the FET has ≥ 2× headroom.
Typical DualShock ERM motors stall somewhere around 0.3–1 A depending on size — hence
the assortment suggestion.

## Already covered — do not buy

RP2040 board (YD-RP2040, flashed & verified) · donor pak · motor (salvage) · USB-C
cable · second RP2040 board inbound (bench debug/probe spare).
