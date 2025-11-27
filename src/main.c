#include "pico/stdlib.h"
#include "hardware/gpio.h"

// TOP half RGB
#define PIN_R1  2
#define PIN_G1  3
#define PIN_B1  4

// BOTTOM half RGB
#define PIN_R2  6
#define PIN_G2  7
#define PIN_B2  8

// Address lines A–E
#define PIN_A   9
#define PIN_B   11
#define PIN_C   12
#define PIN_D   13
#define PIN_E   14

// Control
#define PIN_CLK 16
#define PIN_LAT 18   // OE is tied to GND externally

#define WIDTH 64
#define ROWS 32   // 64x64 panel → 32 row pairs

void init_pins(void) {
    int pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_A, PIN_B, PIN_C, PIN_D, PIN_E,
        PIN_CLK, PIN_LAT
    };

    for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }
}

void set_row(int r) {
    gpio_put(PIN_A, (r >> 0) & 1);
    gpio_put(PIN_B, (r >> 1) & 1);
    gpio_put(PIN_C, (r >> 2) & 1);
    gpio_put(PIN_D, (r >> 3) & 1);
    gpio_put(PIN_E, (r >> 4) & 1);
}

void shift_white_pixel(void) {
    // white on both halves
    gpio_put(PIN_R1, 1);
    gpio_put(PIN_G1, 1);
    gpio_put(PIN_B1, 1);

    gpio_put(PIN_R2, 1);
    gpio_put(PIN_G2, 1);
    gpio_put(PIN_B2, 1);

    gpio_put(PIN_CLK, 1);
    gpio_put(PIN_CLK, 0);
}

void draw_full_white(int ms) {
    for (int t = 0; t < ms; t++) {
        for (int row = 0; row < ROWS; row++) {
            set_row(row);

            // shift 64 white pixels into this rowpair
            for (int x = 0; x < WIDTH; x++) {
                shift_white_pixel();
            }

            // latch
            gpio_put(PIN_LAT, 1);
            gpio_put(PIN_LAT, 0);

            // small hold time per row
            sleep_us(150);
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(500);

    init_pins();

    while (1) {
        draw_full_white(2000);   // ~2 seconds full white
        // clear off-time: just send black
        for (int t = 0; t < 500; t++) {
            for (int row = 0; row < ROWS; row++) {
                set_row(row);
                // shift black pixels
                for (int x = 0; x < WIDTH; x++) {
                    gpio_put(PIN_R1, 0);
                    gpio_put(PIN_G1, 0);
                    gpio_put(PIN_B1, 0);
                    gpio_put(PIN_R2, 0);
                    gpio_put(PIN_G2, 0);
                    gpio_put(PIN_B2, 0);
                    gpio_put(PIN_CLK, 1);
                    gpio_put(PIN_CLK, 0);
                }
                gpio_put(PIN_LAT, 1);
                gpio_put(PIN_LAT, 0);
                sleep_us(150);
            }
        }
    }
}
