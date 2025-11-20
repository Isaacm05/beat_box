#include "pico/stdlib.h"

// Matrix pin definitions (from your mapping)
#define PIN_R1  4
#define PIN_G1  5
#define PIN_B1  6
#define PIN_R2  7
#define PIN_G2  8
#define PIN_B2  9

#define PIN_OE  10   // Output enable (active low)
#define PIN_CLK 11   // Shift clock
#define PIN_LAT 12   // Latch

#define PIN_A   13
#define PIN_B   14
#define PIN_C   15
#define PIN_D   16
#define PIN_E   17

#define PANEL_WIDTH   64
#define ROW_PAIRS     32   // 64 rows total, 1:32 scan

static void matrix_gpio_init(void) {
    int pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_OE, PIN_CLK, PIN_LAT,
        PIN_A, PIN_B, PIN_C, PIN_D, PIN_E
    };

    for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], true); // output
        gpio_put(pins[i], 0);
    }

    // Start with output disabled
    gpio_put(PIN_OE, 1);  // OE is active-low
}

// Set the row address lines A-E for row 0..31
static void set_row_address(uint8_t row) {
    gpio_put(PIN_A, (row >> 0) & 0x1);
    gpio_put(PIN_B, (row >> 1) & 0x1);
    gpio_put(PIN_C, (row >> 2) & 0x1);
    gpio_put(PIN_D, (row >> 3) & 0x1);
    gpio_put(PIN_E, (row >> 4) & 0x1);
}

// Draw one row-pair (top+bottom) with a simple color pattern
static void draw_row_pair(uint8_t row) {
    // Set which row pair we’re drawing
    set_row_address(row);

    // Blank display while we shift in new data
    gpio_put(PIN_OE, 1);
    gpio_put(PIN_LAT, 0);

    for (int col = 0; col < PANEL_WIDTH; col++) {
        // ----- Choose a simple test pattern -----
        // Example:
        //  - top half = solid red
        //  - bottom half = solid green

        bool r1 = true;   // top R
        bool g1 = false;
        bool b1 = false;

        bool r2 = false;  // bottom G
        bool g2 = true;
        bool b2 = false;

        // You can also vary by column/row for fancier tests:
        // r1 = (col & 8) != 0;
        // g2 = (row & 4) != 0;

        gpio_put(PIN_R1, r1);
        gpio_put(PIN_G1, g1);
        gpio_put(PIN_B1, b1);

        gpio_put(PIN_R2, r2);
        gpio_put(PIN_G2, g2);
        gpio_put(PIN_B2, b2);

        // Clock rising edge shifts data into panel’s shift registers
        gpio_put(PIN_CLK, 1);
        // tiny delay to keep timing sane
        asm volatile("nop \n nop \n nop");
        gpio_put(PIN_CLK, 0);
    }

    // Latch shifted data into the LED drivers
    gpio_put(PIN_LAT, 1);
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_LAT, 0);

    // Turn on this row pair for a short time
    gpio_put(PIN_OE, 0);      // enable output (active low)
    sleep_us(100);            // row on-time; tweak for brightness/flicker
    gpio_put(PIN_OE, 1);      // blank again
}

int main() {
    stdio_init_all();
    matrix_gpio_init();

    while (true) {
        // Continuously scan all 32 row pairs
        for (uint8_t row = 0; row < ROW_PAIRS; row++) {
            draw_row_pair(row);
        }
    }

    return 0;
}
