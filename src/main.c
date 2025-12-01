#include "led/led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)

int main() {
    stdio_init_all();

    printf("Initializing 64x64 LED Matrix...\n");
    led_matrix_init();

    // Clear display first
    led_matrix_clear();

    // Test horizontal line on row 0 (top) - RED
    for (int x = 0; x < 64; x++) {
        led_matrix_set_pixel(x, 0, 255, 0, 0);
    }

    // Test horizontal line on row 31 (middle) - GREEN
    for (int x = 0; x < 64; x++) {
        led_matrix_set_pixel(x, 31, 0, 255, 0);
    }

    // Test horizontal line on row 32 (second half start) - BLUE
    for (int x = 0; x < 64; x++) {
        led_matrix_set_pixel(x, 32, 0, 0, 255);
    }

    // Test horizontal line on row 63 (bottom) - WHITE
    for (int x = 0; x < 64; x++) {
        led_matrix_set_pixel(x, 63, 255, 255, 255);
    }

    printf("Starting refresh loop at %d FPS...\n", TARGET_FPS);

    uint64_t last_frame_time = time_us_64();

    while (true) {
        uint64_t current_time = time_us_64();
        uint64_t elapsed = current_time - last_frame_time;

        // Refresh the display
        led_matrix_refresh();

        // // Maintain target frame rate
        // if (elapsed < FRAME_TIME_US) {
        //     sleep_us(FRAME_TIME_US - elapsed);
        // }

        last_frame_time = time_us_64();
    }
}