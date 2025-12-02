#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>
 
#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second
#define DEBOUNCE_US      30000     // 30 ms debounce
#define LONG_PRESS_US   400000     // 400 ms for long-press

const uint BUTTON_PINS[4] = {
    PIN_LEFT,
    PIN_RIGHT,
    PIN_UP,
    PIN_DOWN
};

enum Button {
    BUTTON_LEFT = 0,
    BUTTON_RIGHT = 1,
    BUTTON_UP = 2,
    BUTTON_DOWN = 3
};

volatile bool button_short_press[4] = {0};
volatile bool button_long_press[4]  = {0};

volatile bool button_raw_event[4] = {0};
bool last_raw_state[4]            = {0};
bool stable_state[4]              = {0};
uint64_t last_change_time[4]      = {0};
uint64_t press_start_time[4]      = {0};


const uint8_t colors[][3] = {
        {255,   0,   0},  // Red
        {  0, 255,   0},  // Green
        {  0,   0, 255},  // Blue
        {255, 255,   0},  // Yellow
        {255,   0, 255},  // Magenta
        {  0, 255, 255},  // Cyan
        {255, 255, 255},  // White
        {  0,   0,   0},  // Black
};
const int num_colors = 8;


// moving single pixel scan test
void moving_pixel(void) {
    while (1) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x++) {

                led_matrix_clear();
                led_matrix_set_pixel(x, y, 255, 255, 255);

                uint64_t start = time_us_64();
                while (time_us_64() - start < 50000) {  // 50ms per pixel
                    led_matrix_refresh();
                }
            }
        }
    }
}

// full screen color cycle test
void full_color_cycle(void) {
    int current_color = 0;

    uint64_t last_change = time_us_64();

    while (1) {
        uint64_t now = time_us_64();
        uint32_t start_frame = time_us_32();

        if (now - last_change >= COLOR_CHANGE_INTERVAL_US) {
            led_matrix_fill(colors[current_color][0],
                            colors[current_color][1],
                            colors[current_color][2]);

            current_color = (current_color + 1) % num_colors;
            last_change = now;

            printf("Color index = %d\n", current_color);
        }

        led_matrix_refresh();

        uint32_t elapsed = time_us_32() - start_frame;
        if (elapsed < FRAME_TIME_US)
            sleep_us(FRAME_TIME_US - elapsed);
    }
}

// grid pattern test
void grid_pattern(void) {
    led_matrix_clear();

    // every 8 pixels draw  line
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            bool line =
                (x % 8 == 0) ||   
                (y % 8 == 0);     

            if (line)
                led_matrix_set_pixel(x, y, 255, 255, 255);
        }
    }
}

// RGB stripes test
void rgb_stripes(void) {
    int stripe_height = MATRIX_HEIGHT / 3;

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            if (y < stripe_height)
                led_matrix_set_pixel(x, y, 255, 0, 0);    // red
            else if (y < stripe_height * 2)
                led_matrix_set_pixel(x, y, 0, 255, 0);    // green
            else
                led_matrix_set_pixel(x, y, 0, 0, 255);    // blue
        }
    }
}

// horizontal fade test
void horizontal_fade(void) {
    // 4x4 ordered dithering matrix 
    static const uint8_t bayer[4][4] = {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };

    led_matrix_clear();

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            // Intensity goes from 0–255 across the screen
            uint8_t intensity = (x * 255) / (MATRIX_WIDTH - 1);

            // Map to 0–15 to match the Bayer matrix range
            uint8_t level = intensity >> 4;  // /16

            // Using the 4×4 matrix
            uint8_t threshold = bayer[y % 4][x % 4];

            // ON if intensity level > matrix threshold
            uint8_t value = (level > threshold) ? 255 : 0;

            // White fade
            led_matrix_set_pixel(x, y, value, value, value);
        }
    }
}

// vertical fade test
void vertical_fade(void) {
    // 4x4 ordered dithering matrix 
    static const uint8_t bayer[4][4] = {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };

    led_matrix_clear();

    for (int y = 0; y < MATRIX_HEIGHT; y++) {

        // Intensity goes 0–255 from top to bottom
        uint8_t intensity = (y * 255) / (MATRIX_HEIGHT - 1);

        // Scale intensity 0–255 → 0–15
        uint8_t level = intensity >> 4;

        for (int x = 0; x < MATRIX_WIDTH; x++) {

            // Lookup threshold based on pixel location
            uint8_t threshold = bayer[y % 4][x % 4];

            // Decide whether this pixel is ON or OFF
            uint8_t value = (level > threshold) ? 255 : 0;

            // Draw vertical grayscale fade
            led_matrix_set_pixel(x, y, value, value, value);
        }
    }

}


// button initialization 
void buttons_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]);  // active LOW
    }
}

// button interrupt service routine
void button_isr(uint gpio, uint32_t events) {
    for (int i = 0; i < 4; i++) {
        if (gpio == BUTTON_PINS[i]) {
            button_raw_event[i] = true;
            last_change_time[i] = time_us_64();  
        }
    }
}

// process button state with debounce
void process_button(int b) {
    uint64_t now = time_us_64();

    // Raw event from ISR?
    if (button_raw_event[b]) {
        button_raw_event[b] = false;  
        // actual debounce happens below
    }

    bool raw = (gpio_get(BUTTON_PINS[b]) == 0);

    // Detect changes in raw signal
    if (raw != last_raw_state[b]) {
        last_raw_state[b] = raw;
        last_change_time[b] = now;
    }

    // Not yet debounced
    if (now - last_change_time[b] < DEBOUNCE_US)
        return;

    // State is debounced & stable
    if (raw != stable_state[b]) {
        stable_state[b] = raw;

        if (stable_state[b]) {
            // PRESSED
            press_start_time[b] = now;
        } else {
            // RELEASED — measure duration
            uint64_t press_len = now - press_start_time[b];

            if (press_len >= LONG_PRESS_US) {
                button_long_press[b] = true;
            } else {
                button_short_press[b] = true;
            }
        }
    }
}

// main() — combines LED color cycling + 4-button debounce
int main() {
    stdio_init_all();
    sleep_ms(2000);

    led_matrix_init();
    buttons_init();

    // Install shared ISR for all buttons
    for (int i = 0; i < 4; i++) {
        gpio_set_irq_enabled_with_callback(
            BUTTON_PINS[i],
            GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,   // detect press + release
            true,
            &button_isr
        );
    }

    int current_color = 0;

    // Fill the initial color
    led_matrix_fill(colors[current_color][0],
                    colors[current_color][1],
                    colors[current_color][2]);

    
    while (1) {

        led_matrix_refresh();   

        // Process debouncing + edge detection for all 4 buttons
        for (int b = 0; b < 4; b++) {
            process_button(b);
        }

        // LEFT = previous color
        if (button_short_press[BUTTON_LEFT]) {
            button_short_press[BUTTON_LEFT] = false;  // clear flag

            current_color--;
            if (current_color < 0)
                current_color = num_colors - 1;

            led_matrix_fill(colors[current_color][0],
                            colors[current_color][1],
                            colors[current_color][2]);

            printf("Color changed to: Color %d (LEFT short press)\n",
                   current_color + 1);
        }

        // RIGHT = next color
        if (button_short_press[BUTTON_RIGHT]) {
            button_short_press[BUTTON_RIGHT] = false;

            current_color = (current_color + 1) % num_colors;

            led_matrix_fill(colors[current_color][0],
                            colors[current_color][1],
                            colors[current_color][2]);

            printf("Color changed to: Color %d (RIGHT short press)\n",
                   current_color + 1);
        }
    }
}
