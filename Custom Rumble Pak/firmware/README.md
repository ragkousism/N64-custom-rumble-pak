# Firmware — RP2040 Rumble Pak

RP2040 (Raspberry Pi Pico) firmware that makes a stripped donor board behave like
an N64 Rumble Pak on the controller accessory bus. See [`../DESIGN.md`](../DESIGN.md)
for the hardware design and protocol derivation.

> **Status: draft / pre-bring-up.** The logic is complete but the bus *timing* has
> not been validated on real hardware yet (see [Validation](#validation-on-hardware)).

## How it works

Two PIO state machines on `pio0`, plus a tiny CPU loop:

- **`n64_read` (autonomous):** whenever `/OE` is asserted, it drives **`0x80`** onto
  `D0..D7` and releases the bus (Hi-Z) the moment `/OE` deasserts. This is what the
  console's detection probe reads back to recognise a rumble pak. The `0x80` is
  latched into the pin output register once at startup; the hot loop only flips the
  data-bus direction, so the `/OE`-to-data path is a single instruction.
- **`n64_write`:** on each `/WE` strobe it snapshots `GP0..GP19` (data + strobes +
  region address) and pushes it to the RX FIFO.
- **CPU loop (`main.c`):** blocks on the write FIFO; when a write hits the motor
  region (`0xC000-0xFFFF`, i.e. `A15=1 && A14=1` with `/CE` low) it mirrors **D0**
  to the motor FET gate (`1`=on, `0`=off).

No SRAM is emulated — a rumble pak has no save memory.

## Pin map

| Signal | GPIO | Notes |
|---|---|---|
| D0..D7 | GP0..GP7 | bidirectional data bus (PIO-driven) |
| /CE | GP8 | chip enable (in) |
| /OE | GP9 | read strobe (in) |
| /WE | GP10 | write strobe (in) |
| A0..A4 | GP11..GP15 | block offset (kept for robustness; unused in decode) |
| A13 | GP16 | guard (unused in decode) |
| A14 | GP17 | region select |
| A15 | GP18 | region select |
| A12 | GP19 | guard (unused in decode) |
| MOTOR_EN | GP22 | FET gate, active high |
| DETECT | — | tied to Pico `3V3 OUT` in hardware, not a GPIO |

Power is from the NiMH battery into VSYS; the console 3.3 V rail is never connected.

## Build

Requires the [Pico SDK](https://github.com/raspberrypi/pico-sdk) and the ARM
toolchain.

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -B build -S .
cmake --build build
```

Output: `build/n64_rumble.uf2`.

## Flash

Hold **BOOTSEL**, plug the Pico into USB, copy the `.uf2` to the `RPI-RP2` drive.
(Flash on the bench, not while the pak is inserted in a console.)

## Validation on hardware

These are the open items the draft cannot resolve without a scope/logic analyzer
(also tracked in `../DESIGN.md`):

1. **Read window** — confirm the `/OE`-low → data-valid time the controller expects,
   and that the single-instruction drive path meets it. If marginal, the default
   125 MHz clock gives the most headroom.
2. **Write data-valid point** — `n64_write` samples shortly after `/WE` goes low
   (with a small settle delay). If D0 capture is flaky, move the sample to just
   before `/WE` rises.
3. **/CE qualification** — the read path keys on `/OE` alone (normal SRAM controller
   behaviour). If `/OE` is seen asserted outside an enabled cycle, gate it on `/CE`.
4. **Detection** — verify a real N64 + rumble game (or a libdragon rumble test)
   actually detects and drives the pak.

## Possible refinements (later)

- Lower the system clock for less idle battery draw (verify the read path still
  meets timing).
- Drive the motor with PWM to soften/scale rumble, or to compensate for the ~2.4 V
  NiMH pack vs the motor's ~3 V rating.
