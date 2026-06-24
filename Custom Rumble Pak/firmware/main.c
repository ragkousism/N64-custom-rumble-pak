/*
 * N64 Rumble Pak emulation on the controller accessory bus (RP2040).
 *
 * See ../DESIGN.md for the full design and ../WIRING.md for the solder map.
 * In short: this board is stripped to the bare edge connector of a donor
 * Controller Pak; the RP2040 sits on the bus and behaves like a rumble pak:
 *
 *   - Read responder (PIO, autonomous): drive 0x80 on every read so the
 *     console's detection probe sees a rumble pak.
 *   - Write sampler (PIO) + this CPU loop: on a write to the motor region
 *     (0xC000-0xFFFF), mirror D0 to the motor FET gate.
 *
 * The Pico is powered from its own NiMH battery (VSYS). It NEVER draws power
 * from the console 3.3V rail. DETECT (EC1 pin 14) is tied to our own 3V3 OUT in
 * hardware (not handled here).
 *
 * Build options:
 *   -DRUMBLE_DEBUG=ON  ->  USB-CDC logging of every bus write + a heartbeat.
 *                          For bench bring-up only; do NOT flash this into the
 *                          finished pak (no USB host, wastes power).
 *
 * STATUS: draft. The bus timing (read window, write data-valid point) must be
 * validated on real hardware with a logic analyzer before this is trusted.
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "n64_rumble.pio.h"

#ifdef RUMBLE_DEBUG
#include <stdio.h>
#endif

/* ---- Pin map (must match n64_rumble.pio and ../WIRING.md) ---- */
#define PIN_D0     0    /* GP0..GP7  = D0..D7 data bus            */
#define PIN_CE     8    /* /CE  (active low)                      */
#define PIN_OE     9    /* /OE  (active low) read strobe          */
#define PIN_WE     10   /* /WE  (active low) write strobe         */
#define PIN_A0     11   /* GP11..GP15 = A0..A4                    */
#define PIN_A13    16
#define PIN_A14    17
#define PIN_A15    18
#define PIN_A12    19
#define PIN_MOTOR  22   /* MOTOR_EN -> FET gate (active high)     */

#define SNAPSHOT_BITS 20  /* GP0..GP19 captured by the write SM   */

#define ID_READ_VALUE 0x80u  /* value returned on reads (D7 high) */

/* Decode whether a snapshot targets the motor region and what D0 is. */
static inline bool is_motor_write(uint32_t s, bool *d0_out) {
    bool ce  = (s >> PIN_CE)  & 1u;   /* 0 = chip enabled                 */
    bool a14 = (s >> PIN_A14) & 1u;
    bool a15 = (s >> PIN_A15) & 1u;
    *d0_out  = (s >> PIN_D0)  & 1u;   /* 0x01 -> on, 0x00 -> off          */
    /* Motor region 0xC000-0xFFFF == A15=1 && A14=1, with /CE asserted.   */
    return (!ce && a15 && a14);
}

static void init_read_sm(PIO pio, uint sm, uint offset) {
    pio_sm_config c = n64_read_program_get_default_config(offset);
    sm_config_set_out_pins(&c, PIN_D0, 8);
    /* shift right so `out pins,8` takes the low 8 bits; no autopull */
    sm_config_set_out_shift(&c, true, false, 32);

    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio, PIN_D0 + i);
    }
    /* data bus starts as inputs (Hi-Z) until the first read */
    pio_sm_set_consecutive_pindirs(pio, sm, PIN_D0, 8, false);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    /* the program pulls this constant once and latches it onto D0..D7 */
    pio_sm_put_blocking(pio, sm, ID_READ_VALUE);
}

static void init_write_sm(PIO pio, uint sm, uint offset) {
    pio_sm_config c = n64_write_program_get_default_config(offset);
    sm_config_set_in_pins(&c, PIN_D0);                 /* IN base = GP0 */
    /* shift left so GP0->bit0 ... GP19->bit19; manual push (no autopush) */
    sm_config_set_in_shift(&c, false, false, 32);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#ifdef RUMBLE_DEBUG
static const char *region_name(uint32_t s) {
    bool a14 = (s >> PIN_A14) & 1u;
    bool a15 = (s >> PIN_A15) & 1u;
    if (a15 && a14)  return "MOTOR";   /* 0xC000-0xFFFF */
    if (a15 && !a14) return "ID";      /* 0x8000-0xBFFF */
    return "SAVE";                     /* 0x0000-0x7FFF */
}
#endif

int main(void) {
    /* Strobe + address lines are inputs the PIO reads. Plain SIO inputs are
     * fine -- PIO `wait gpio` / `in pins` sample the pad regardless of funcsel. */
    for (int p = PIN_CE; p <= PIN_A12; p++) {  /* GP8..GP19 */
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
    }

    /* Motor output, forced off at boot (a 100k gate pulldown also holds it off
     * during the Hi-Z window before this runs). */
    gpio_init(PIN_MOTOR);
    gpio_put(PIN_MOTOR, 0);
    gpio_set_dir(PIN_MOTOR, GPIO_OUT);

    PIO pio = pio0;
    uint off_read  = pio_add_program(pio, &n64_read_program);
    uint off_write = pio_add_program(pio, &n64_write_program);
    const uint sm_read  = 0;
    const uint sm_write = 1;

    init_read_sm(pio, sm_read, off_read);
    init_write_sm(pio, sm_write, off_write);

#ifndef RUMBLE_DEBUG
    /* Production: the read responder runs entirely in PIO. This loop only
     * services writes, blocking (and idling the core) until one arrives. */
    while (true) {
        uint32_t s = pio_sm_get_blocking(pio, sm_write);  /* GP0..GP19 */
        bool d0;
        if (is_motor_write(s, &d0)) {
            gpio_put(PIN_MOTOR, d0);
        }
    }
#else
    /* Debug: USB-CDC logging. stdio_init_all() does NOT block waiting for a
     * host, so the pak still works if nothing is connected. We poll the write
     * FIFO (so we can also emit a heartbeat) and log every snapshot. */
    stdio_init_all();
    printf("\nN64 rumble pak firmware (DEBUG build) up. Waiting for bus...\n");

    absolute_time_t next_hb = make_timeout_time_ms(1000);
    uint32_t writes = 0;
    bool motor = false;

    while (true) {
        while (!pio_sm_is_rx_fifo_empty(pio, sm_write)) {
            uint32_t s = pio_sm_get(pio, sm_write);
            bool d0;
            if (is_motor_write(s, &d0)) {
                gpio_put(PIN_MOTOR, d0);
                motor = d0;
            }
            writes++;
            printf("WR %-5s data=0x%02X  CE=%u OE=%u WE=%u  A15=%u A14=%u  motor=%u\n",
                   region_name(s),
                   (unsigned)(s & 0xFFu),
                   (unsigned)((s >> PIN_CE) & 1u),
                   (unsigned)((s >> PIN_OE) & 1u),
                   (unsigned)((s >> PIN_WE) & 1u),
                   (unsigned)((s >> PIN_A15) & 1u),
                   (unsigned)((s >> PIN_A14) & 1u),
                   (unsigned)motor);
        }
        if (absolute_time_diff_us(get_absolute_time(), next_hb) <= 0) {
            printf("[hb] alive  writes=%lu  motor=%u\n", (unsigned long)writes, (unsigned)motor);
            next_hb = make_timeout_time_ms(1000);
        }
    }
#endif
}
