# N64
Nintendo 64 (N64) related projects

I'm a hobbyist Nintendo tinkerer and repairer. I'll upload N64 related things here if I think others might benefit from them.

The first project is a schematic for an N64 Rumble Pak.

## Custom RP2040 Rumble Pak (this fork)

This fork adds a project that converts a **broken aftermarket N64 Controller (Memory)
Pak into a working Rumble Pak**.

A Memory Pak and a Rumble Pak plug into the same controller expansion port and share
the same SRAM-style bus, but internally they are different: a Memory Pak is just an
SRAM chip, while a Rumble Pak decodes the console's "rumble" command and switches a
vibration motor. So a dead Memory Pak can't rumble on its own — but its board breaks
out the whole accessory bus, which makes it a useful donor.

The plan strips the donor board down to its bare edge connector and adds:

- a **Raspberry Pi Pico (RP2040)** that sits on the bus, answers the console's
  accessory-detection probe, and decodes the motor on/off command (its PIO is fast
  enough to meet the original SRAM's read timing); and
- a small **motor driver stage** (logic-level MOSFET, flyback diode, fuse) powered by
  its own batteries — never from the console's 3.3 V rail.

Full design notes, the edge-connector pinout, the Pico GPIO map, the power
architecture, and the open questions are in
[`Custom Rumble Pak/DESIGN.md`](Custom%20Rumble%20Pak/DESIGN.md).

This is a work in progress. The original Rumble Pak schematic this builds on lives in
[`N64 Rumble Pak/`](N64%20Rumble%20Pak/) and comes from the upstream project
([Ugly-Mug/N64](https://github.com/Ugly-Mug/N64)).
