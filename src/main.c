#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second


// 1) static single pixel test
void test_single_pixel(uint16_t x, uint16_t y) {
    led_matrix_clear();
    led_matrix_set_pixel(x, y, 255, 0, 0); // red pixel

    // Keep refreshing forever
    while (1) {
        led_matrix_refresh();
    }
}


// 2) moving single pixel scan test
void test_moving_pixel(void) {
    while (true) {
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


// 3) full screen color cycle test
void test_color_cycle(void) {
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


// 4) grid pattern test
void test_grid_pattern(void) {
    led_matrix_clear();

    // Every 8 pixels: draw a line
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            bool line =
                (x % 8 == 0) ||   // vertical grid line
                (y % 8 == 0);     // horizontal grid line

            if (line)
                led_matrix_set_pixel(x, y, 255, 255, 255);
        }
    }

    // Display grid forever
    while (1) {
        led_matrix_refresh();
    }
}


// 5) RGB stripes test
void test_rgb_stripes(void) {
    int stripe_height = MATRIX_HEIGHT / 3;

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            if (y < stripe_height)
                led_matrix_set_pixel(x, y, 255, 0, 0);    // Red stripe
            else if (y < stripe_height * 2)
                led_matrix_set_pixel(x, y, 0, 255, 0);    // Green stripe
            else
                led_matrix_set_pixel(x, y, 0, 0, 255);    // Blue stripe
        }
    }

    while (1) {
        led_matrix_refresh();
    }
}


// 6) gradient test
void test_gradient(void) {
    led_matrix_clear();

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {

            // Create binary "pseudogradient" using thresholds
            uint8_t r = (x % 8) < 4 ? 255 : 0;
            uint8_t g = (y % 8) < 4 ? 255 : 0;
            uint8_t b = ((x + y) % 8) < 4 ? 255 : 0;

            led_matrix_set_pixel(x, y, r, g, b);
        }
    }

    while (1) {
        led_matrix_refresh();
    }
}



// main - set up to run any one of the tests
int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("Initializing LED matrix...\n");
    led_matrix_init();

    // Pick ONE of these to run:

    // 1. Display a single pixel at (20, 10)
    // test_single_pixel(20, 10);

    // 2. Move a pixel across the entire panel
    // test_moving_pixel();

    // 3. Full-screen color cycle
    //test_color_cycle();

    // 4. Grid pattern test
    //test_grid_pattern();  

    // 5. RGB stripes test
    //test_rgb_stripes();

    // 6. Gradient test
    // this one doesn't really work but looks cool
    //test_gradient();   
    
    // 7. Text rendering test
    //test_text();


    // Should never reach here
    while (1) { }
}
