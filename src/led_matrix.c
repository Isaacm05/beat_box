#include "led_matrix.h"
#include <string.h>

uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];


// 5x7 bitmap font: each character is 5 columns, each column = 7 bits
// Only SPACE (index 0), ! (index 1), digits, and uppercase letters implemented
static const uint8_t font5x7[][5] = {

    // SPACE (index 0)
    {0x00,0x00,0x00,0x00,0x00},

    // ! (index 1)
    {0x00,0x00,0x5F,0x00,0x00},

    // '0'..'9' (indexes 2..11)
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9

    // A-Z (indexes 12..37)
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x26,0x49,0x49,0x49,0x32}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
};


static void draw_char(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b)
{
    int index;

    if (c == ' ') {
        index = 0;
    }
    else if (c == '!') {
        index = 1;
    }
    else if (c >= '0' && c <= '9') {
        index = 2 + (c - '0');             // digits = 2..11
    }
    else if (c >= 'A' && c <= 'Z') {
        index = 12 + (c - 'A');            // A-Z
    }
    else {
        return; // unsupported character
    }

    const uint8_t *glyph = font5x7[index];

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                led_matrix_set_pixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void led_matrix_draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b)
{
    int cursor_x = x;

    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(cursor_x, y, text[i], r, g, b);
        cursor_x += 6;   // 5px glyph + 1px spacing
    }
}

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

