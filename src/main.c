#include "pico/stdlib.h"
#include "hardware/gpio.h"

// TOP half RGB
#define PIN_R1  5
#define PIN_G1  6
#define PIN_B1  7

// BOTTOM half RGB
#define PIN_R2  8
#define PIN_G2  9
#define PIN_B2  10

// Address lines A–E
#define PIN_A   11
#define PIN_B   12
#define PIN_C   13
#define PIN_D   14
#define PIN_E   15

// Control
#define PIN_CLK 16
#define PIN_OE  17
#define PIN_LAT 18  

#define WIDTH 64
#define ROWS 32

void init_pins(void) {
    int pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_A, PIN_B, PIN_C, PIN_D, PIN_E,
        PIN_CLK, PIN_OE, PIN_LAT
    };

    for (int i = 0; i < 14; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }
}

void set_row(int r) {
    gpio_put(PIN_A, (r & 1));
    gpio_put(PIN_B, (r >> 1) & 1);
    gpio_put(PIN_C, (r >> 2) & 1);
    gpio_put(PIN_D, (r >> 3) & 1);
    gpio_put(PIN_E, (r >> 4) & 1);
}

void shift_white_pixel(void) {
    gpio_put(PIN_R1, 1);
    gpio_put(PIN_G1, 1);
    gpio_put(PIN_B1, 1);

    gpio_put(PIN_R2, 1);
    gpio_put(PIN_G2, 1);
    gpio_put(PIN_B2, 1);

    gpio_put(PIN_CLK, 1);
    gpio_put(PIN_CLK, 0);
}

void draw_full_white(void) {
    for (int row = 0; row < ROWS; row++) {
        gpio_put(PIN_OE, 1);  // disable output while shifting

        set_row(row);

        for (int x = 0; x < WIDTH; x++) {
            shift_white_pixel();
        }

        // Latch new row
        gpio_put(PIN_LAT, 1);
        gpio_put(PIN_LAT, 0);

        gpio_put(PIN_OE, 0);   // enable output for this row

        // HOLD the row so it becomes visible
        sleep_us(40);          // *** this controls brightness and refresh ***
    }
}

void configure_display(void)
{
    gpio_put(PIN_CLK, 0);
    gpio_put(PIN_OE, 0);
    gpio_put(PIN_LAT, 0);
}

// Write two FM6126A control registers:
// 1. Register 0x0C = 0x00 (unlock 1)
// 2. Register 0x0B = 0x00 (unlock 2)

void fm6126a_write(uint8_t reg, uint8_t value) {
    // Send 13 bits: AAAAA DDDDDDDDD
    // Panel ignores B1/B2 for this command, but uses R1 as data.

    for (int i = 12; i >= 0; i--) {
        int bit = ( (reg << 8) | value ) >> i & 1;
        gpio_put(PIN_R1, bit);
        gpio_put(PIN_CLK, 1);
        gpio_put(PIN_CLK, 0);
    }
    gpio_put(PIN_LAT, 1);
    gpio_put(PIN_LAT, 0);
}

void fm6126a_reset() {
    fm6126a_write(0x0C, 0x00);
    fm6126a_write(0x0B, 0x00);
}


int main() {

    printf("Hello World\n");
    sleep_ms(200);
    init_pins();
    fm6126a_reset();
    configure_display();

    while (1) {
        draw_full_white();
    }
}
