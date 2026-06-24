# Wiring / Solder Map

Where every wire goes for the RP2040 rumble pak. Pairs with
[`DESIGN.md`](DESIGN.md) (rationale) and [`firmware/`](firmware/) (pin map).

All connections are 3.3 V logic. **The Pico is powered only from the battery — never
connect the console 3.3 V (EC1 pins 15/31) to anything.**

## Conventions

- **EC1 pin** = pad on the stripped donor edge connector (32-pin card edge).
- **Pico pin** = the physical header pin number (1–40) on a Raspberry Pi Pico.
- **GPIO** = the RP2040 GP number the firmware uses.
- Before wiring, **confirm which end of the edge connector is pin 1** with a
  multimeter: EC1 pins 1 and 17 are GND — find them by continuity to the donor's
  ground plane, and number from there.

> Exactness matters on the data bus: **Dn must go to GPn** (D0→GP0 … D7→GP7), or the
> `0x80` the firmware drives will appear scrambled (0x80 = D7 high, D0–D6 low).

## 1. Power, ground, detect (always required)

| From | To | Notes |
|---|---|---|
| Battery + → PTC fuse → switch → **VBATT** node | Pico **VSYS** = pin **39** | 2× AAA NiMH (~2.4 V). Onboard buck-boost makes 3.3 V. |
| VBATT node | Motor stage (see §4) | motor runs off raw battery |
| Battery − | Pico **GND** (pin 3/8/13/18/23/28/38) | common ground |
| EC1 **1, 17** (GND) | Pico **GND** | tie console ground to common ground |
| EC1 **14** (DETECT) | Pico **3V3 OUT** = pin **36** | signals "present" using *our* 3.3 V |
| EC1 **15, 31** (3.3 V) | **leave unconnected** | never touch the console rail |

Do **not** connect Pico **VBUS (pin 40)**.

## 2. Bus signals — REQUIRED (13 wires)

These are the only bus lines the firmware actually uses.

| EC1 pin | Signal | GPIO | Pico pin |
|:---:|:---|:---:|:---:|
| 12 | D0 | GP0 | 1 |
| 13 | D1 | GP1 | 2 |
| 16 | D2 | GP2 | 4 |
| 32 | D3 | GP3 | 5 |
| 30 | D4 | GP4 | 6 |
| 29 | D5 | GP5 | 7 |
| 28 | D6 | GP6 | 9 |
| 27 | D7 | GP7 | 10 |
| 19 | /CE | GP8 | 11 |
| 25 | /OE | GP9 | 12 |
| 20 | /WE | GP10 | 14 |
| 2 | A14 | GP17 | 22 |
| 18 | A15 | GP18 | 24 |

## 3. Bus signals — OPTIONAL (extra address lines)

Wired in the design "for robustness" but **not used by the current firmware**. Solder
these only if you later extend the decode; otherwise skip them.

| EC1 pin | Signal | GPIO | Pico pin |
|:---:|:---|:---:|:---:|
| 11 | A0 | GP11 | 15 |
| 10 | A1 | GP12 | 16 |
| 9 | A2 | GP13 | 17 |
| 8 | A3 | GP14 | 19 |
| 7 | A4 | GP15 | 20 |
| 21 | A13 | GP16 | 21 |
| 3 | A12 | GP19 | 25 |

**Not connected at all** (controller outputs the firmware ignores): EC1 pins 4 (A7),
5 (A6), 6 (A5), 22 (A8), 23 (A9), 24 (A11), 26 (A10).

- *Optional protection:* a 33–100 Ω resistor in series on each bus wire (between EC1
  and the Pico). Helps with contention/ESD; skippable at these speeds.

## 4. Motor driver stage

Build on a small protoboard near the Pico. FET = AO3400A (logic-level N-MOSFET);
**verify the gate/source/drain pinout against your FET's datasheet** (AO3400A SOT-23 is
1=Gate, 2=Source, 3=Drain).

```
                          VBATT (battery+ after fuse+switch)
                            |
                +-----------+-----------+----------------> Pico VSYS (pin 39)
                |           |           |
            [Cbulk      [motor      [Dflyback cathode]
            100-470uF]  term B]     (SS14/1N5819)
                |           |           |
               GND      [MOTOR]    [Dflyback anode]
                            |           |
                          term A ---+---+---> FET DRAIN
                            |
                         [100nF across motor terminals]

  Pico GP22 (pin 29) ---[100 ohm]---+--- FET GATE
                                    |
                                 [100k]
                                    |
                                   GND --- FET SOURCE --- Battery -  (all common GND)
```

Connections in list form:
- **Pico GP22 (pin 29)** → 100 Ω → **FET gate**
- **FET gate** → 100 kΩ → **GND** (holds motor off at boot)
- **FET source** → **GND**
- **FET drain** → **motor terminal A**
- **Motor terminal B** → **VBATT**
- **Flyback diode** across motor: anode → terminal A (drain), cathode → terminal B (VBATT)
- **100 nF** across motor terminals A–B
- **VSYS decoupling:** 100–470 µF bulk + 10 µF + 100 nF from VBATT to GND, near the Pico
- **PTC fuse** in the battery + lead, sized just above motor stall current
- **Slide switch** in the battery + lead (also avoids back-powering when uninserted)

## 5. Quick sanity checks before powering on

1. Continuity: every EC1 data/strobe pin reaches the correct Pico pin; **Dn ↔ GPn** in
   order.
2. No short between VBATT and GND; no wire to EC1 15/31 or Pico VBUS.
3. DETECT (EC1 14) reads ~3.3 V relative to GND once the battery is on.
4. Motor off at rest (gate pulled low); motor spins when GP22 is forced high.
5. Flash the **debug** build, open the USB serial console, and watch for the `[hb]`
   heartbeat and `WR` lines when inserted in a console — that confirms the bus wiring
   before you trust a game.
