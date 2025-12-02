#include "led_matrix.h"
#include <string.h>
 
uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];

#define RGB_MASK ((1u << PIN_R1) | (1u << PIN_G1) | (1u << PIN_B1) | \
                  (1u << PIN_R2) | (1u << PIN_G2) | (1u << PIN_B2))


static const uint8_t font3x5[][5] = {
    // SPACE
    {0b000,0b000,0b000,0b000,0b000},

    // ! 
    {0b010,0b010,0b010,0b000,0b010},

    // Digits 0–9
    {0b111,0b101,0b101,0b101,0b111}, // 0
    {0b010,0b110,0b010,0b010,0b111}, // 1
    {0b111,0b001,0b111,0b100,0b111}, // 2
    {0b111,0b001,0b111,0b001,0b111}, // 3
    {0b101,0b101,0b111,0b001,0b001}, // 4
    {0b111,0b100,0b111,0b001,0b111}, // 5
    {0b111,0b100,0b111,0b101,0b111}, // 6
    {0b111,0b001,0b001,0b001,0b001}, // 7
    {0b111,0b101,0b111,0b101,0b111}, // 8
    {0b111,0b101,0b111,0b001,0b111}, // 9

    // A–Z
    {0b111,0b101,0b111,0b101,0b101}, // A
    {0b110,0b101,0b110,0b101,0b110}, // B
    {0b111,0b100,0b100,0b100,0b111}, // C
    {0b110,0b101,0b101,0b101,0b110}, // D
    {0b111,0b100,0b111,0b100,0b111}, // E
    {0b111,0b100,0b111,0b100,0b100}, // F
    {0b111,0b100,0b101,0b101,0b111}, // G
    {0b101,0b101,0b111,0b101,0b101}, // H
    {0b111,0b010,0b010,0b010,0b111}, // I
    {0b111,0b001,0b001,0b101,0b111}, // J
    {0b101,0b110,0b100,0b110,0b101}, // K
    {0b100,0b100,0b100,0b100,0b111}, // L
    {0b101,0b111,0b111,0b101,0b101}, // M
    {0b101,0b111,0b111,0b111,0b101}, // N
    {0b111,0b101,0b101,0b101,0b111}, // O
    {0b111,0b101,0b111,0b100,0b100}, // P
    {0b111,0b101,0b101,0b111,0b001}, // Q
    {0b111,0b101,0b111,0b101,0b101}, // R
    {0b111,0b100,0b111,0b001,0b111}, // S
    {0b111,0b010,0b010,0b010,0b010}, // T
    {0b101,0b101,0b101,0b101,0b111}, // U
    {0b101,0b101,0b101,0b101,0b010}, // V
    {0b101,0b101,0b111,0b111,0b101}, // W
    {0b101,0b101,0b010,0b101,0b101}, // X
    {0b101,0b101,0b010,0b010,0b010}, // Y
    {0b111,0b001,0b010,0b100,0b111}, // Z
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
        index = 2 + (c - '0');
    }
    else if (c >= 'A' && c <= 'Z') {
        index = 12 + (c - 'A');
    }
    else {
        return;
    }

    const uint8_t *glyph = font3x5[index];

    // 3x5 font: 3 columns wide, 5 rows tall
    for (int row = 0; row < 5; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 3; col++) {
            if (bits & (1 << (2 - col))) {
                led_matrix_set_pixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void led_matrix_draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b)
{
    int cursor_x = x;

    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(cursor_x, y, text[i], r, g, b); // calls helper function
        cursor_x += 4; // extra 1px for spacing
    }
}

void led_matrix_scroll_text(const char *text, int y, uint8_t r, uint8_t g, uint8_t b, int speed_us)
{
    // Compute pixel width of the entire string
    int text_length = 0;
    for (const char *p = text; *p; p++)
        text_length++;

    int text_width = text_length * 6;  // extra 1px for spacing

    // Start from the right edge of the screen
    int x_offset = MATRIX_WIDTH;

    while (1) {
        led_matrix_clear();

        int cursor_x = x_offset;

        // Draw characters shifted by x_offset
        for (int i = 0; text[i] != '\0'; i++) {

            char c = text[i];

            draw_char(cursor_x, y, c, r, g, b);
            cursor_x += 6;
        }

        // Refresh panel
        led_matrix_refresh();

        // Move text left 1 pixel
        x_offset--;

        // Restart scroll if the text is fully off-screen
        if (x_offset + text_width < 0) {
            x_offset = MATRIX_WIDTH;
        }

        // Control scroll speed
        sleep_us(speed_us);
    }
}

void led_matrix_draw_text_rainbow(int x, int y, const char *text) {
    static const uint8_t rainbow[][3] = {
        {255,   0,   0}, // Red
        {255, 255,   0}, // Yellow 
        {  0, 255,   0}, // Green
        {  0, 255, 255}, // Cyan 
        {  0,   0, 255}, // Blue
        {255,   0, 255}, // Magenta 
        {255, 255, 255}, // White
    };
    const int num_colors = 7;

    int cursor_x = x;
    int i = 0;

    while (*text) {
        char c = *text++;

        uint8_t r = rainbow[i % num_colors][0];
        uint8_t g = rainbow[i % num_colors][1];
        uint8_t b = rainbow[i % num_colors][2];

        draw_char(cursor_x, y, c, r, g, b);

        cursor_x += 6;  // 5px glyph + 1px spacing
        i++;
    }
}

void led_matrix_scroll_text_rainbow(const char *text, int speed_us)
{
    // Same 7-color rainbow palette
    static const uint8_t rainbow[][3] = {
        {255,   0,   0}, // Red
        {255, 255,   0}, // Yellow
        {  0, 255,   0}, // Green
        {  0, 255, 255}, // Cyan
        {  0,   0, 255}, // Blue
        {255,   0, 255}, // Magenta
        {255, 255, 255}, // White
    };
    const int num_colors = 7;

    // Compute full pixel width of the text
    int text_length = 0;
    for (const char *p = text; *p; p++) text_length++;
    int text_width = text_length * 6;   // extra 1px for spacing

    // Start off-screen to the right
    int x_offset = MATRIX_WIDTH;

    while (1) {
        led_matrix_clear();

        int cursor_x = x_offset;
        int color_i = 0;

        // Draw each character shifted by x_offset
        for (int i = 0; text[i] != '\0'; i++) {

            char c = text[i];

            // Pick a rainbow color
            uint8_t r = rainbow[color_i % num_colors][0];
            uint8_t g = rainbow[color_i % num_colors][1];
            uint8_t b = rainbow[color_i % num_colors][2];

            // Draw character at the shifted pos
            draw_char(cursor_x, 20, c, r, g, b);

            cursor_x += 6;
            color_i++;
        }

        // Refresh panel
        led_matrix_refresh();

        // Move left 1 pixel
        x_offset--;

        // If fully off-screen, restart
        if (x_offset + text_width < 0) {
            x_offset = MATRIX_WIDTH;
        }

        // Control scroll speed
        sleep_us(speed_us);
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
    gpio_put(PIN_CLK, 1);
    sleep_us(1); 
    gpio_put(PIN_CLK, 0);
}

static inline void latch_pulse(void) {
    gpio_put(PIN_LAT, 1);
    sleep_us(1);
    gpio_put(PIN_LAT, 0);
}

void led_matrix_refresh(void) {
    for (int row = 0; row < SCAN_ROWS; row++) {
        // Disable output while we shift new data
        gpio_put(PIN_OE, 1);

        // Select which row pair we're about to display
        set_row_address(row);

        // Shift out 64 pixels for this row pair
        for (int col = 0; col < MATRIX_WIDTH; col++) {
            // Upper half 
            uint8_t r1 = framebuffer[row][col][0] ? 1 : 0;
            uint8_t g1 = framebuffer[row][col][1] ? 1 : 0;
            uint8_t b1 = framebuffer[row][col][2] ? 1 : 0;

            // Lower half 
            uint8_t r2 = framebuffer[row + SCAN_ROWS][col][0] ? 1 : 0;
            uint8_t g2 = framebuffer[row + SCAN_ROWS][col][1] ? 1 : 0;
            uint8_t b2 = framebuffer[row + SCAN_ROWS][col][2] ? 1 : 0;

            // Set data pins 
            uint32_t bits =
            (r1 << PIN_R1) |
            (g1 << PIN_G1) |
            (b1 << PIN_B1) |
            (r2 << PIN_R2) |
            (g2 << PIN_G2) |
            (b2 << PIN_B2);

            gpio_put_masked(RGB_MASK, bits);

            clock_pulse();
        }

        // Latch shifted data into output registers
        latch_pulse();

        // Turn the row pair ON for some time 
        gpio_put(PIN_OE, 0);
        sleep_us(100);
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

void led_matrix_draw_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    int x = radius;
    int y = 0;
    int decision = 1 - radius;

    while (y <= x) {

        led_matrix_set_pixel(cx + x, cy + y, r, g, b);
        led_matrix_set_pixel(cx + y, cy + x, r, g, b);
        led_matrix_set_pixel(cx - y, cy + x, r, g, b);
        led_matrix_set_pixel(cx - x, cy + y, r, g, b);

        led_matrix_set_pixel(cx - x, cy - y, r, g, b);
        led_matrix_set_pixel(cx - y, cy - x, r, g, b);
        led_matrix_set_pixel(cx + y, cy - x, r, g, b);
        led_matrix_set_pixel(cx + x, cy - y, r, g, b);

        y++;

        if (decision <= 0) {
            decision += 2*y + 1;
        } else {
            x--;
            decision += 2*(y - x) + 1;
        }
    }
}

void led_matrix_fill_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b)
{
    int x = radius;
    int y = 0;
    int decision = 1 - radius;

    while (y <= x) {

        for (int i = cx - x; i <= cx + x; i++) {
            led_matrix_set_pixel(i, cy + y, r, g, b);
            led_matrix_set_pixel(i, cy - y, r, g, b);
        }

        for (int i = cx - y; i <= cx + y; i++) {
            led_matrix_set_pixel(i, cy + x, r, g, b);
            led_matrix_set_pixel(i, cy - x, r, g, b);
        }

        y++;

        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            x--;
            decision += 2 * (y - x) + 1;
        }
    }
}

void led_matrix_draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    // Top horizontal line
    for (int i = 0; i < w; i++) {
        led_matrix_set_pixel(x + i, y, r, g, b);
    }

    // Bottom horizontal line
    for (int i = 0; i < w; i++) {
        led_matrix_set_pixel(x + i, y + h - 1, r, g, b);
    }

    // Left vertical line
    for (int j = 0; j < h; j++) {
        led_matrix_set_pixel(x, y + j, r, g, b);
    }

    // Right vertical line
    for (int j = 0; j < h; j++) {
        led_matrix_set_pixel(x + w - 1, y + j, r, g, b);
    }
}

void led_matrix_fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            led_matrix_set_pixel(x + i, y + j, r, g, b);
        }
    }
}
