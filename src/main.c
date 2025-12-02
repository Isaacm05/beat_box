#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second

int main() {
    stdio_init_all();
    sleep_ms(2000); // USB time

    printf("Initializing 64x64 LED Matrix...\n");
    led_matrix_init();

    // Color cycle array: {R, G, B}
    const uint8_t colors[][3] = {
        {255, 0, 0},     // Red
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {255, 255, 0},   // Yellow
        {255, 0, 255},   // Magenta
        {0, 255, 255},   // Cyan
        {255, 255, 255},  // White
        {0, 0, 0,}       // Black
    };
    const int num_colors = 8;
    int current_color = 0;

    uint64_t last_color_change = time_us_64();

    while (1) {
        uint64_t current_time = time_us_64();
        uint32_t start_time = time_us_32();

        // Check if it's time to change color
        if (current_time - last_color_change >= COLOR_CHANGE_INTERVAL_US) {
            led_matrix_fill(colors[current_color][0], 
                           colors[current_color][1], 
                           colors[current_color][2]);
            current_color = (current_color + 1) % num_colors;
            last_color_change = current_time;
            printf("Color: %d\n", current_color);
        }

        // Refresh the LED matrix
        led_matrix_refresh();

        // Frame timing
        uint32_t elapsed = time_us_32() - start_time;
        if (elapsed < FRAME_TIME_US) {
            sleep_us(FRAME_TIME_US - elapsed);
        }   
    }
}
