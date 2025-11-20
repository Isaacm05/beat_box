#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// =========================================================
// FINAL PIN MAPPING (YOUR NEW CONNECTIONS)
// =========================================================

// --- TOP Row RGB ---
#define PIN_G1  4
#define PIN_R1  5
#define PIN_B1  6

// --- BOTTOM Row RGB ---
#define PIN_G2  7
#define PIN_R2  8
#define PIN_B2  9

// --- Address Lines ---
#define PIN_A   10
#define PIN_B   11
#define PIN_C   12
#define PIN_D   13
#define PIN_E   14

// --- Control Pins ---
#define PIN_LAT 15
#define PIN_CLK 16
#define PIN_OE  17

// =========================================================

#define WIDTH       64
#define HEIGHT      64
#define ROW_PAIRS   32

// Simple RGB pixel
typedef struct {
    uint8_t r, g, b;
} Pixel;

// Framebuffer
Pixel fb[HEIGHT][WIDTH];

// ---------------------------------------------------------
// INITIALIZE PINS
// ---------------------------------------------------------
void matrix_gpio_init() {
    int pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_A, PIN_B, PIN_C, PIN_D, PIN_E,
        PIN_LAT, PIN_CLK, PIN_OE
    };

    for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }

    gpio_put(PIN_OE, 1);  // OE is active LOW → start OFF
}

// ---------------------------------------------------------
// SET ROW ADDRESS LINES
// ---------------------------------------------------------
void set_row_address(uint8_t row) {
    gpio_put(PIN_A, (row >> 0) & 1);
    gpio_put(PIN_B, (row >> 1) & 1);
    gpio_put(PIN_C, (row >> 2) & 1);
    gpio_put(PIN_D, (row >> 3) & 1);
    gpio_put(PIN_E, (row >> 4) & 1);
}

// ---------------------------------------------------------
// SCAN ONE ROW-PAIR
// ---------------------------------------------------------
void scan_rowpair(uint8_t rp) {
    set_row_address(rp);

    // Disable output while shifting
    gpio_put(PIN_OE, 1);

    for (int x = 0; x < WIDTH; x++) {
        Pixel top = fb[rp][x];
        Pixel bot = fb[rp + ROW_PAIRS][x];

        // Output TOP row RGB
        gpio_put(PIN_R1, top.r > 0);
        gpio_put(PIN_G1, top.g > 0);
        gpio_put(PIN_B1, top.b > 0);

        // Output BOTTOM row RGB
        gpio_put(PIN_R2, bot.r > 0);
        gpio_put(PIN_G2, bot.g > 0);
        gpio_put(PIN_B2, bot.b > 0);

        // CLOCK (safe rising edge)
        gpio_put(PIN_CLK, 1);
        asm volatile("nop; nop; nop;");
        gpio_put(PIN_CLK, 0);
    }

    // Some panels require an extra dummy column
    gpio_put(PIN_CLK, 1);
    gpio_put(PIN_CLK, 0);

    // Latch delayed (important for most HUB75E panels)
    sleep_us(1);
    gpio_put(PIN_LAT, 1);
    sleep_us(1);
    gpio_put(PIN_LAT, 0);

    // Turn row ON briefly
    gpio_put(PIN_OE, 0);
    sleep_us(150);   // brightness / refresh control
    gpio_put(PIN_OE, 1);
}

// ---------------------------------------------------------
// CLEAR FRAMEBUFFER
// ---------------------------------------------------------
void clear_fb() {
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            fb[y][x] = (Pixel){0,0,0};
}

// ---------------------------------------------------------
// DRAW MOVING VERTICAL LINE
// ---------------------------------------------------------
void draw_line(int xpos) {
    clear_fb();

    if (xpos < 0 || xpos >= WIDTH) return;

    for (int y = 0; y < HEIGHT; y++) {
        fb[y][xpos].r = 255;
        fb[y][xpos].g = 255;
        fb[y][xpos].b = 255;   // white vertical line
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main() {
    stdio_init_all();
    sleep_ms(1000);
    printf("=== MOVING LINE TEST START ===\n");

    matrix_gpio_init();

    int xpos = 0;

    while (1) {
        draw_line(xpos);

        // Scan whole frame several times for persistence
        for (int i = 0; i < 200; i++) {
            for (int rp = 0; rp < ROW_PAIRS; rp++) {
                scan_rowpair(rp);
            }
        }

        xpos = (xpos + 1) % WIDTH;
    }

    return 0;
}
