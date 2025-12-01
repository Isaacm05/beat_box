// led_matrix.c
#include "led_matrix.h"
#include <string.h>

uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];

void led_matrix_init(void) {
    // RGB pins - upper half
    gpio_init(PIN_R1);
    gpio_set_dir(PIN_R1, GPIO_OUT);
    gpio_put(PIN_R1, 0);

    gpio_init(PIN_G1);
    gpio_set_dir(PIN_G1, GPIO_OUT);
    gpio_put(PIN_G1, 0);

    gpio_init(PIN_B1);
    gpio_set_dir(PIN_B1, GPIO_OUT);
    gpio_put(PIN_B1, 0);

    // RGB pins - lower half
    gpio_init(PIN_R2);
    gpio_set_dir(PIN_R2, GPIO_OUT);
    gpio_put(PIN_R2, 0);

    gpio_init(PIN_G2);
    gpio_set_dir(PIN_G2, GPIO_OUT);
    gpio_put(PIN_G2, 0);

    gpio_init(PIN_B2);
    gpio_set_dir(PIN_B2, GPIO_OUT);
    gpio_put(PIN_B2, 0);

    // Address pins
    gpio_init(PIN_A);
    gpio_set_dir(PIN_A, GPIO_OUT);
    gpio_put(PIN_A, 0);

    gpio_init(PIN_B);
    gpio_set_dir(PIN_B, GPIO_OUT);
    gpio_put(PIN_B, 0);

    gpio_init(PIN_C);
    gpio_set_dir(PIN_C, GPIO_OUT);
    gpio_put(PIN_C, 0);

    gpio_init(PIN_D);
    gpio_set_dir(PIN_D, GPIO_OUT);
    gpio_put(PIN_D, 0);

    gpio_init(PIN_E);
    gpio_set_dir(PIN_E, GPIO_OUT);
    gpio_put(PIN_E, 0);

    // Control pins
    gpio_init(PIN_CLK);
    gpio_set_dir(PIN_CLK, GPIO_OUT);
    gpio_put(PIN_CLK, 0);

    gpio_init(PIN_OE);
    gpio_set_dir(PIN_OE, GPIO_OUT);
    gpio_put(PIN_OE, 1);  // active low

    gpio_init(PIN_LAT);
    gpio_set_dir(PIN_LAT, GPIO_OUT);
    gpio_put(PIN_LAT, 0);

    led_matrix_clear();
}

static inline void set_row_address(uint8_t row) {
    gpio_put(PIN_A, (row >> 0) & 1);
    gpio_put(PIN_B, (row >> 1) & 1);
    gpio_put(PIN_C, (row >> 2) & 1);
    gpio_put(PIN_D, (row >> 3) & 1);
    gpio_put(PIN_E, (row >> 4) & 1);
}

static inline void clock_pulse(void) {
    gpio_put(PIN_CLK, 1);
    gpio_put(PIN_CLK, 0);
}

static inline void latch_pulse(void) {
    gpio_put(PIN_LAT, 1);
    gpio_put(PIN_LAT, 0);
}

void led_matrix_refresh(void) {
    // Binary Code Modulation for color depth
    for (int bit = 0; bit < COLOR_DEPTH; bit++) {
        uint8_t bit_mask = 1 << bit;

        for (int row = 0; row < SCAN_ROWS; row++) {
            // Disable output while shifting
            gpio_put(PIN_OE, 1);

            // Shift out pixel data for this row
            for (int col = 0; col < MATRIX_WIDTH; col++) {
                // Upper half pixel (row)
                uint8_t r1 = (framebuffer[row][col][0] & bit_mask) ? 1 : 0;
                uint8_t g1 = (framebuffer[row][col][1] & bit_mask) ? 1 : 0;
                uint8_t b1 = (framebuffer[row][col][2] & bit_mask) ? 1 : 0;

                // Lower half pixel (row + 32)
                uint8_t r2 = (framebuffer[row + SCAN_ROWS][col][0] & bit_mask) ? 1 : 0;
                uint8_t g2 = (framebuffer[row + SCAN_ROWS][col][1] & bit_mask) ? 1 : 0;
                uint8_t b2 = (framebuffer[row + SCAN_ROWS][col][2] & bit_mask) ? 1 : 0;

                // Set RGB pins
                gpio_put(PIN_R1, r1);
                gpio_put(PIN_G1, g1);
                gpio_put(PIN_B1, b1);
                gpio_put(PIN_R2, r2);
                gpio_put(PIN_G2, g2);
                gpio_put(PIN_B2, b2);

                // Clock in the pixel
                clock_pulse();
            }

            // Set row address
            set_row_address(row);

            // Latch the data
            latch_pulse();

            gpio_put(PIN_OE, 0);
            sleep_us((1 << bit) * 10); // 10x multiplier
        }
    }
}

void led_matrix_set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT) {
        framebuffer[y][x][0] = r;
        framebuffer[y][x][1] = g;
        framebuffer[y][x][2] = b;
    }
}

void led_matrix_clear(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}

void led_matrix_fill(uint8_t r, uint8_t g, uint8_t b) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            framebuffer[y][x][0] = r;
            framebuffer[y][x][1] = g;
            framebuffer[y][x][2] = b;
        }
    }
}