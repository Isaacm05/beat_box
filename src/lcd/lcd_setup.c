#include "lcd_setup.h"

#define PIN_SDI 43
#define PIN_CS 25
#define PIN_SCK 42
#define PIN_DC 37
#define PIN_nRESET 38

void setup_spi_lcd() {
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
    gpio_set_function(PIN_DC, GPIO_FUNC_SIO);
    gpio_set_function(PIN_nRESET, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_nRESET, GPIO_OUT);

    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_nRESET, 1);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);

    // Increased from 12MHz to 40MHz for faster LCD refresh
    spi_init(spi1, 40000000);
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void setup_lcd() {
    stdio_init_all();
    setup_spi_lcd();
    LCD_Setup();
    LCD_Clear(0x0000);
}
