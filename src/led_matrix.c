#include "led_matrix.h"
#include <string.h>

uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];

void led_matrix_init(void) {
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

    // OE is active low: start with display OFF
    gpio_put(PIN_OE, 1);

    // Clear framebuffer to black
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
    // Fast pulse: panel is fine with MHz-level clocks
    gpio_put(PIN_CLK, 1);
    sleep_us(1); // brief delay
    gpio_put(PIN_CLK, 0);
}

static inline void latch_pulse(void) {
    // Fast latch pulse
    gpio_put(PIN_LAT, 1);
    sleep_us(1); // brief delay
    gpio_put(PIN_LAT, 0);
}

void led_matrix_refresh(void) {
    for (int row = 0; row < SCAN_ROWS; row++) {
        // 1) Disable output while we shift new data
        gpio_put(PIN_OE, 1);

        // 2) Select which row pair we're about to display
        set_row_address(row);

        // 3) Shift out 64 pixels for this row pair
        for (int col = 0; col < MATRIX_WIDTH; col++) {
            // Upper half (rows 0–31)
            uint8_t r1 = framebuffer[row][col][0] ? 1 : 0;
            uint8_t g1 = framebuffer[row][col][1] ? 1 : 0;
            uint8_t b1 = framebuffer[row][col][2] ? 1 : 0;

            // Lower half (rows 32–63)
            uint8_t r2 = framebuffer[row + SCAN_ROWS][col][0] ? 1 : 0;
            uint8_t g2 = framebuffer[row + SCAN_ROWS][col][1] ? 1 : 0;
            uint8_t b2 = framebuffer[row + SCAN_ROWS][col][2] ? 1 : 0;

            gpio_put(PIN_R1, r1);
            gpio_put(PIN_G1, g1);
            gpio_put(PIN_B1, b1);
            gpio_put(PIN_R2, r2);
            gpio_put(PIN_G2, g2);
            gpio_put(PIN_B2, b2);

            clock_pulse();
        }

        // 4) Latch shifted data into output registers
        latch_pulse();

        // 5) Turn the row pair ON for some time (brightness)
        gpio_put(PIN_OE, 0);
        sleep_us(450);   // tweak between ~100–500 for brightness vs flicker
    }
}

void led_matrix_set_pixel(uint16_t x, uint16_t y,
                          uint8_t r, uint8_t g, uint8_t b) {
    if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT) {
        framebuffer[y][x][0] = r;
        framebuffer[y][x][1] = g;
        framebuffer[y][x][2] = b;
    }
}

void led_matrix_clear(void) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            framebuffer[y][x][0] = 0;
            framebuffer[y][x][1] = 0;
            framebuffer[y][x][2] = 0;
        }
    }
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

