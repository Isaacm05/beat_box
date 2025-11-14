// #include "pico/stdlib.h"
// #include "hardware/spi.h"
// #include "lcd/lcd.h"
// #include <stdio.h>
// #include <string.h>
// #include <math.h>   
// // #include  <unistd.h>

// /****************************************** */
// #define PIN_SDI    43
// #define PIN_CS     25
// #define PIN_SCK    42
// #define PIN_DC     37
// #define PIN_nRESET 38
// #define M_PI 3.14159265358979323846
// #define HEIGHT 
// #define WIDTH 

// // Uncomment the following #define when 
// // you are ready to run Step 3.

// // WARNING: The process will take a VERY 
// // long time as it compiles and uploads 
// // all the image frames into the uploaded 
// // binary!  Expect to wait 5 minutes.
// // #define ANIMATION

// /****************************************** */
// #ifdef ANIMATION
// #include "images.h"
// #endif
// /****************************************** */

// void init_spi_lcd() {
//     gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
//     gpio_set_function(PIN_DC, GPIO_FUNC_SIO);
//     gpio_set_function(PIN_nRESET, GPIO_FUNC_SIO);

//     gpio_set_dir(PIN_CS, GPIO_OUT);
//     gpio_set_dir(PIN_DC, GPIO_OUT);
//     gpio_set_dir(PIN_nRESET, GPIO_OUT);

//     gpio_put(PIN_CS, 1); // CS high
//     gpio_put(PIN_DC, 0); // DC low
//     gpio_put(PIN_nRESET, 1); // nRESET high

//     // initialize SPI1 with 48 MHz clock
//     gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
//     gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);
//     spi_init(spi1, 12000000);
//     spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
// }

// Picture* load_image(const char* image_data);
// void free_image(Picture* pic);

// int main() {
//     stdio_init_all();

//     init_spi_lcd();

//     LCD_Setup();
//     LCD_Clear(0x0000); // Clear the screen to black


//     #ifndef ANIMATION
//     #define N_BODIES   3      // Number of bodies in the simulation
//     #define G          12.0f  // Gravitational constant
//     #define DT         0.01f // Simulation time step
//     #define SOFTENING  5.0f   // Prevents extreme forces at close range

//     // Colors as per the 16-bit RGB565 specification.
//     #define BLACK      0x0000
//     #define RED        0xF800
//     #define LIME       0x07E0   // brighter green
//     #define BLUE       0x001F

//     // Make things easier to keep track of for each "body".
//     typedef struct {
//         float x, y, vx, vy, mass;
//         uint16_t color;
//     } Body;

//     // sample sin wave 
//     int frequency = 10.0f;
//     static float samples[44100];
//     for (int i = 0; i < 44100; i++) {
//         samples[i] = sinf(2 * M_PI * frequency * i / 44100.0f);
//     }

//     LCD_PlotWaveform(samples, 44100, frequency, 100);
//     #endif

//     for(;;);
// }