#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// === Matrix pin definitions ===
#define PIN_R1  4
#define PIN_G1  5
#define PIN_B1  6
#define PIN_R2  7
#define PIN_G2  8
#define PIN_B2  9

#define PIN_OE  10
#define PIN_CLK 11
#define PIN_LAT 12

#define PIN_A   13
#define PIN_B   14
#define PIN_C   15
#define PIN_D   16
#define PIN_E   17

#define PANEL_WIDTH   64
#define ROW_PAIRS     32

void matrix_gpio_init(void) {
    int pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_OE, PIN_CLK, PIN_LAT,
        PIN_A, PIN_B, PIN_C, PIN_D, PIN_E
    };

    for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }

    // Start with OE disabled (panel dark)
    gpio_put(PIN_OE, 1);
}

void test_pins_blink() {
    printf("[TEST] Blinking OE / CLK / LAT...\n");

    for (int i = 0; i < 20; i++) {
        gpio_put(PIN_OE, i & 1);
        gpio_put(PIN_CLK, i & 1);
        gpio_put(PIN_LAT, (i >> 1) & 1);

        sleep_ms(200);
        printf("  OE=%d CLK=%d LAT=%d\n",
               i & 1, i & 1, (i >> 1) & 1);
    }

    printf("[TEST] Blink done.\n");
}

void test_single_row() {
    printf("[TEST] Lighting one pixel per row...\n");

    for (int row = 0; row < 32; row++) {
        // Set row address
        gpio_put(PIN_A, (row >> 0) & 1);
        gpio_put(PIN_B, (row >> 1) & 1);
        gpio_put(PIN_C, (row >> 2) & 1);
        gpio_put(PIN_D, (row >> 3) & 1);
        gpio_put(PIN_E, (row >> 4) & 1);

        printf("  Row %d address: %d%d%d%d%d\n",
            row,
            gpio_get(PIN_E), gpio_get(PIN_D),
            gpio_get(PIN_C), gpio_get(PIN_B), gpio_get(PIN_A)
        );

        // Shift exactly one pixel (top-left red)
        gpio_put(PIN_R1, 1);
        gpio_put(PIN_G1, 0);
        gpio_put(PIN_B1, 0);

        gpio_put(PIN_CLK, 1);
        sleep_us(2);
        gpio_put(PIN_CLK, 0);

        // Latch
        gpio_put(PIN_LAT, 1);
        sleep_us(2);
        gpio_put(PIN_LAT, 0);

        // Flash row ON for 5 ms
        gpio_put(PIN_OE, 0);
        sleep_ms(5);
        gpio_put(PIN_OE, 1);

        sleep_ms(50);
    }

    printf("[TEST] Row test done.\n");
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Give USB time to enumerate

    printf("\n=== HUB75 MATRIX DEBUG START ===\n");

    matrix_gpio_init();
    printf("GPIO init done.\n");

    // TEST 1: Blink timing control pins
    test_pins_blink();

    // TEST 2: Light a single pixel on each row
    test_single_row();

    printf("=== DEBUG DONE ===\n");

    while (true) {
        sleep_ms(500);
        printf("Running idle...\n");
    }

    return 0;
}
