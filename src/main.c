// #include "led/led_matrix.h"
// #include "pico/stdlib.h"
// #include <stdio.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)


#include "led/led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>

int main() {
    stdio_init_all();
    sleep_ms(2000); // USB time

    printf("Initializing 64x64 LED Matrix...\n");
    led_matrix_init();

    // Fill framebuffer with white

    printf("fill blue\n");

    // led_matrix_fill(0, 0, 255);

    led_matrix_set_pixel(10, 10, 0, 0, 255);
    
    while(true)
        led_matrix_refresh();

    
   


    // //sleep_ms(2000);
    // led_matrix_refresh();

    // printf("clear");
    // led_matrix_clear();
    // sleep_ms(5000);
    // led_matrix_refresh();

    // printf("fill red");
    // led_matrix_fill(255, 0, 0);  // full R G B
    // sleep_ms(2000);
    // led_matrix_refresh();
    clock_t start_time = clock();

    
    // while (true) {
    //     //if ((clock() - start_time) / CLOCKS_PER_SEC >= 1) {
    //         //start_time = clock();
    //        // led_matrix_fill(0, 255, 0);

    //     //}   
        
    //     // led_matrix_fill(255, 0, 0);
    //     // led_matrix_refresh();
    //     // led_matrix_fill(0, 255, 0);
    //     // led_matrix_refresh();   
    //     // led_matrix_fill(0, 0, 255);
    //     led_matrix_refresh();
    // }
}



// int main() {
//     stdio_init_all();
//     sleep_ms(2000); // give USB serial time to enumerate

//     printf("Initializing 64x64 LED Matrix...\n");
//     led_matrix_init();

//     // --- SET ENTIRE SCREEN TO WHITE ---
//     for (int y = 0; y < MATRIX_HEIGHT; y++) {
//         for (int x = 0; x < MATRIX_WIDTH; x++) {
//             led_matrix_set_pixel(x, y, 255, 255, 255);  // full R G B
//         }
//     }

//     printf("Screen set to WHITE. Starting refresh loop...\n");

//     uint64_t last_frame_time = time_us_64();

//     while (true) {
//         uint64_t now = time_us_64();
//         uint64_t elapsed = now - last_frame_time;

//         // Push framebuffer to the panel
//         led_matrix_refresh();

//         // Hold framerate
//         if (elapsed < FRAME_TIME_US) {
//             sleep_us(FRAME_TIME_US - elapsed);
//         }

//         last_frame_time = time_us_64();
//     }
// }
