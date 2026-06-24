# Custom RP2040 Rumble Pak — Design Notes

Goal: convert a broken aftermarket **N64 Controller (Memory) Pak** into a working
**Rumble Pak** by reusing its edge connector and driving a motor with our own
microcontroller. This document captures the plan and the open questions; it is a
living design doc, not a finished build.

> Status: **planning / pre-build.** Hardware design is committed; firmware and the
> protocol-dependent details are still open (see [Open Items](#open-items)).

## Background: why a Memory Pak is not a Rumble Pak

From the console's side, both accessories share the **same physical edge connector**
and the **same parallel, SRAM-style bus**. The N64 *controller's* own chip does the
serial↔parallel translation, so to any accessory the world looks like a memory-mapped
device: address lines `A0–A15`, data `D0–D7`, strobes `/CE` `/WE` `/OE`, plus
`3.3V` / `GND` / `DETECT`.

What hangs off that bus differs completely:

| | Memory (Controller) Pak | Rumble Pak |
|---|---|---|
| Brain | 32 KB SRAM + backup battery | Custom decode/latch chip |
| Output | Stores / returns bytes | Drives a motor via a transistor |
| Power | Coin cell (SRAM retention) | AAA battery (motor) |

So the Memory Pak has **no rumble logic and no motor driver** — but its board breaks
out the entire accessory bus to the SRAM pads, which makes it a good **donor** for the
connector and bus fan-out.

## Donor board identification

Aftermarket (3rd-party) N64 Controller Pak, bank-switched:

- **U1:** `GM76C8128CLLFW70` — LG/Goldstar **128 KB SRAM** (128K×8, 1 Mbit, 70 ns),
  date code `9811`.
- **CR2032** coin cell (SRAM data-retention backup).
- A 74HC-family logic gate (chip-select / battery write-protect gating) and a small
  `U3`; a pushbutton `S1`. The 128 KB SRAM + glue + button indicates a **multi-page /
  bank-switched** pak (128 KB paged as several virtual 32 KB paks).

Because the bank-switch glue sits in the address/CE path, we will **not** rely on the
original SRAM to answer the rumble detection probe. The plan strips the board down to
the **bare edge connector** and lets the MCU own the bus entirely.

### Photos

| Front (SRAM side) | Front (alt) | Back (battery / logic side) |
|---|---|---|
| ![front](photos/donor-board-front.jpg) | ![front-alt](photos/donor-board-front-alt.jpg) | ![back](photos/donor-board-back.jpg) |

(Web-sized copies; the camera originals are ~200 MP and were downsized to keep the
public repo lean.)

## EC1 — accessory port edge-connector pinout

32-pin card edge (from the upstream KiCad schematic). Sorted by pin number:

| Pin | Net | Pin | Net |
|----:|-----|----:|-----|
| 1 | GND | 17 | GND |
| 2 | A14 | 18 | A15 |
| 3 | A12 | 19 | /CE (chip enable) |
| 4 | A7 | 20 | /WE (write) |
| 5 | A6 | 21 | A13 |
| 6 | A5 | 22 | A8 |
| 7 | A4 | 23 | A9 |
| 8 | A3 | 24 | A11 |
| 9 | A2 | 25 | /OE (read/output enable) |
| 10 | A1 | 26 | A10 |
| 11 | A0 | 27 | D7 |
| 12 | D0 | 28 | D6 |
| 13 | D1 | 29 | D5 |
| 14 | DETECT | 30 | D4 |
| 15 | 3.3V | 31 | 3.3V |
| 16 | D2 | 32 | D3 |

- Power: `3.3V` = 15, 31 · `GND` = 1, 17
- Address `A0–A15`: 11,10,9,8,7,6,5,4 (A0–A7), 22,23,26,24 (A8–A11), 3,21,2,18 (A12–A15)
- Data `D0–D7`: 12,13,16,32,30,29,28,27
- Control: `/CE` 19, `/WE` 20, `/OE` 25
- `DETECT`: 14

## MCU choice: RP2040 (Raspberry Pi Pico)

Two hard constraints drove the choice:

1. **3.3 V I/O is mandatory.** The bus is 3.3 V; a 5 V part can damage the console's
   controller IC. (Rules out classic 5 V Arduino Nano/Uno.)
2. **Reads must be answered within ~70 ns.** Once the SRAM is removed, the MCU drives
   the data bus on reads of the ID region. A 16 MHz AVR (62.5 ns/instruction) cannot
   decode an address and drive data in that window. This needs a deterministic
   hardware bus engine.

The **RP2040** satisfies both: 3.3 V native, ~26 usable GPIO, and **PIO** state
machines for cycle-exact bus response. Proven in the N64 homebrew scene. RP2350 /
Teensy 4.x / STM32F4 (FSMC) would also work; RP2040 is the cheapest and simplest.

## Power architecture

**Do not power the Pico from the console 3.3 V rail** — that rail is sized for a tiny
logic chip (a few mA) and an RP2040 can pull 30–50 mA, risking a console brownout.

```
 Battery pack --[F1 fuse]--+-------------> Pico VSYS (pin 39)  -> onboard buck-boost makes 3.3 V
   (>= 1.8 V, see note)    |
                           +------> Motor+ (raw battery, switched by FET)
 Battery- -----------------------> common GND  -- tied to EC1 GND (pins 1, 17)

 EC1 3.3 V (pins 15, 31): leave UNCONNECTED to the Pico.
```

- The Pico board's onboard regulator is a **buck-boost (RT6150), VSYS 1.8–5.5 V** — feed
  the battery into **VSYS** and it makes 3.3 V regardless. Do not also connect VBUS.
- **Battery:** VSYS needs ≥ 1.8 V, so a single 1.5 V cell will not run the Pico. Use
  **at least 2 cells** (2×AA/AAA ≈ 3 V is ideal; a 3.7 V Li-ion also works). The motor
  runs at the pack voltage, so choose a motor rated for it.
- **Motor:** an ERM rumble motor salvaged from a PlayStation (DualShock) controller is
  a good fit — they are ~3 V nominal, which pairs with a 2-cell (≈3 V) pack. The large
  "heavy" motor gives strong low-frequency rumble (higher stall current); the small
  motor gives a lighter buzz. Either works; size the FET and PTC fuse to the chosen
  motor's stall current.
- **Common ground** between battery, Pico, and the console bus is mandatory.
- Tradeoff: the Pico draws idle current from the battery even when not rumbling;
  mitigate in firmware (clock-down / dormant mode between bus accesses).

## Bus → Pico GPIO map

Data bus on **GP0–GP7 contiguous** so PIO can drive/read a byte in one instruction.

| Signal | EC1 pin | Pico GPIO | Dir |
|---|---|---|---|
| D0 | 12 | GP0 | bidir |
| D1 | 13 | GP1 | bidir |
| D2 | 16 | GP2 | bidir |
| D3 | 32 | GP3 | bidir |
| D4 | 30 | GP4 | bidir |
| D5 | 29 | GP5 | bidir |
| D6 | 28 | GP6 | bidir |
| D7 | 27 | GP7 | bidir |
| /CE | 19 | GP8 | in |
| /OE (read) | 25 | GP9 | in |
| /WE (write) | 20 | GP10 | in |
| A0 | 11 | GP11 | in |
| A1 | 10 | GP12 | in |
| A2 | 9 | GP13 | in |
| A3 | 8 | GP14 | in |
| A4 | 7 | GP15 | in |
| A13 | 21 | GP16 | in |
| A14 | 2 | GP17 | in |
| A15 | 18 | GP18 | in |
| A12 | 3 | GP19 | in |
| MOTOR_EN | — | GP22 | out |
| GND | 1, 17 | GND pins | — |
| DETECT | 14 | *TBD (open item #2)* | — |
| 3.3 V | 15, 31 | *unconnected* | — |

**Why only 8 of 16 address lines?** For rumble we need the 32-byte block offset
(`A0–A4`) plus enough high bits to tell the regions apart: the ID/probe block and the
motor block differ in `A14`, and `A15` marks "accessory region vs save region";
`A12/A13` are guard bits. This leaves **GP20, GP21, GP26–GP28 free** for `A5–A11` if
the confirmed protocol needs them. Final address set is locked once Open Item #1 is
resolved.

- Add **33–100 Ω in series** on each bus line between EC1 and the Pico for
  contention/ESD protection — negligible at these speeds.

## Motor driver stage (low-side, logic-level FET)

```
 Battery+ -[F1]-+---------------+---------> Pico VSYS
                |            [Motor JP1]
              (Cbulk          |   ^
              47-100uF)    +---+--+ D1 flyback (SS14 / 1N5819),
                |          | =||= |  cathode -> Motor+ side
                |          +---+--+
                |          Drain
 GP22 -[100R]---+-- Gate -- Q1: AO3400A (logic-level N-MOSFET)
                |          Source
              [100k]           |
              pulldown         |
                +--------------+---------> common GND
```

- **AO3400A** (or similar logic-level N-FET): 3.3 V gate drive fully enhances it and it
  handles a vibration motor easily. (A 2N7002 only suits a very small ~200 mA motor.)
- **Gate pulldown 100 kΩ → GND** keeps the motor **off at power-up**, while the Pico
  GPIOs are still Hi-Z inputs before firmware runs. Important.
- **Flyback Schottky across the motor is mandatory** — the inductive kick otherwise
  destroys the FET and injects noise onto the bus.
- **100 nF ceramic across the motor terminals** for brush noise; **100 nF + 10 µF at
  VSYS**.
- **Fuse** sized just above motor stall current (a resettable PTC is convenient).

## Firmware shape (sketch — to be finalized)

Two PIO state machines on one RP2040:

- **Write SM:** waits for `/CE` & `/WE` active, samples address + data, hands to the
  CPU. CPU updates its model — an echo-shadow for the ID block, and the **motor bit →
  GP22** for the control block.
- **Read SM:** on `/CE` & `/OE` with an address in our region, drives `D0–D7` with a
  precomputed value within the ~70 ns window, then tristates. PIO is what makes the
  window achievable.

Low-power: clock-down / dormant between accesses to limit idle battery drain.

## Added-parts BOM

RP2040 Pico · AO3400A FET · SS14/1N5819 Schottky · 100 Ω gate R + 100 kΩ pulldown ·
~20× 33–100 Ω bus resistors · PTC fuse · 47–100 µF + 100 nF (motor) + 10 µF + 100 nF
(VSYS) · 2-cell battery holder.

## Open Items

1. **Exact ID-probe address + value, and motor-control address + bit.** Decides the
   address decode, whether reads must *echo* the written value or can return a *fixed*
   byte, and the final address-line set. Sources: libdragon rumble / controller-pak
   code, and the bitbuilt.net controller-expansion-port thread cited by the upstream
   schematic.
2. **DETECT (EC1 pin 14) handling** — tied to GND/3.3 V, or driven? Confirm from the
   bus docs and wire accordingly.
3. Confirm the reduced address decode (A0–A4, A12–A15) is sufficient, or extend to the
   reserved spare GPIO.

## References

- Upstream schematic: `../N64 Rumble Pak/` (this repo's fork parent, `Ugly-Mug/N64`).
- N64 controller expansion-port pinout: bitbuilt.net forums (credited in the upstream
  v2 schematic title block).
- libdragon (N64 homebrew SDK) — authoritative for the accessory protocol.
