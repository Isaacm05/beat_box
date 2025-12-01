#include "led/led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)

int main() {
    stdio_init_all();

    printf("Initializing 64x64 LED Matrix...\n");
    for (;;) {
        sleep_ms(1000);
    }
    // led_matrix_init();

    // // Test pattern: red gradient
    // for (int y = 0; y < MATRIX_HEIGHT; y++) {
    //     for (int x = 0; x < MATRIX_WIDTH; x++) {
    //         led_matrix_set_pixel(x, y, x * 4, y * 4, 0);
    //     }
    // }

    // printf("Starting refresh loop at %d FPS...\n", TARGET_FPS);

    // uint64_t last_frame_time = time_us_64();

    // while (true) {
    //     uint64_t current_time = time_us_64();
    //     uint64_t elapsed = current_time - last_frame_time;

    //     // Refresh the display
    //     led_matrix_refresh();

    //     // Maintain target frame rate
    //     if (elapsed < FRAME_TIME_US) {
    //         sleep_us(FRAME_TIME_US - elapsed);
    //     }

    //     last_frame_time = time_us_64();
    // }
}