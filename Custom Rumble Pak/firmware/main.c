/*
 * N64 Rumble Pak emulation on the controller accessory bus (RP2040).
 *
 * See ../DESIGN.md for the full design. In short: this board is stripped to the
 * bare edge connector of a donor Controller Pak; the RP2040 sits on the bus and
 * behaves like a rumble pak:
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
 * STATUS: draft. The bus timing (read window, write data-valid point) must be
 * validated on real hardware with a logic analyzer before this is trusted.
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "n64_rumble.pio.h"

/* ---- Pin map (must match n64_rumble.pio) ---- */
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

    /* The read responder runs entirely in PIO. This loop only services writes,
     * blocking (and idling the core) until a write snapshot arrives. */
    while (true) {
        uint32_t s = pio_sm_get_blocking(pio, sm_write);  /* GP0..GP19 */

        bool ce  = (s >> PIN_CE)  & 1u;   /* 0 = chip enabled        */
        bool a14 = (s >> PIN_A14) & 1u;
        bool a15 = (s >> PIN_A15) & 1u;
        bool d0  = (s >> PIN_D0)  & 1u;   /* 0x01 -> on, 0x00 -> off */

        /* Motor region 0xC000-0xFFFF == A15=1 && A14=1, with /CE asserted */
        if (!ce && a15 && a14) {
            gpio_put(PIN_MOTOR, d0);
        }
    }
}
