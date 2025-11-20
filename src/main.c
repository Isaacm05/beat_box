#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// --- Matrix Pins ---
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

#define WIDTH   64
#define ROWS    64
#define ROW_PAIRS 32

// Simple RGB structure
typedef struct {
    uint8_t r, g, b;
} Pixel;

// Framebuffer: 64x64 RGB pixels
Pixel fb[ROWS][WIDTH];

void matrix_gpio_init() {
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
    gpio_put(PIN_OE, 1);
}

void set_row(int row) {
    gpio_put(PIN_A, (row >> 0) & 1);
    gpio_put(PIN_B, (row >> 1) & 1);
    gpio_put(PIN_C, (row >> 2) & 1);
    gpio_put(PIN_D, (row >> 3) & 1);
    gpio_put(PIN_E, (row >> 4) & 1);
}

void scan_rowpair(int rp) {
    set_row(rp);

    gpio_put(PIN_OE, 1);
    gpio_put(PIN_LAT, 0);

    for (int x = 0; x < WIDTH; x++) {
        Pixel top = fb[rp][x];
        Pixel bot = fb[rp + ROW_PAIRS][x];

        gpio_put(PIN_R1, top.r > 0);
        gpio_put(PIN_G1, top.g > 0);
        gpio_put(PIN_B1, top.b > 0);

        gpio_put(PIN_R2, bot.r > 0);
        gpio_put(PIN_G2, bot.g > 0);
        gpio_put(PIN_B2, bot.b > 0);

        gpio_put(PIN_CLK, 1);
        asm volatile("nop; nop; nop;");
        gpio_put(PIN_CLK, 0);
    }

    gpio_put(PIN_LAT, 1);
    asm volatile("nop");
    gpio_put(PIN_LAT, 0);

    gpio_put(PIN_OE, 0);
    sleep_us(150);
    gpio_put(PIN_OE, 1);
}

void fill_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < WIDTH; x++)
            fb[y][x].r = r, fb[y][x].g = g, fb[y][x].b = b;
}

int main() {
    stdio_init_all();
    sleep_ms(1500);

    printf("=== MATRIX FRAMEBUFFER TEST START ===\n");
    matrix_gpio_init();

    // Start with red
    fill_color(255, 0, 0);
    sleep_ms(500);

    // Then green
    fill_color(0, 255, 0);
    sleep_ms(500);

    // Then blue
    fill_color(0, 0, 255);
    sleep_ms(500);

    // Cycle RGB forever
    while (true) {
        fill_color(255, 0, 0);
        for (int i = 0; i < 300; i++) for (int rp = 0; rp < ROW_PAIRS; rp++) scan_rowpair(rp);

        fill_color(0, 255, 0);
        for (int i = 0; i < 300; i++) for (int rp = 0; rp < ROW_PAIRS; rp++) scan_rowpair(rp);

        fill_color(0, 0, 255);
        for (int i = 0; i < 300; i++) for (int rp = 0; rp < ROW_PAIRS; rp++) scan_rowpair(rp);
    }

    return 0;
}
