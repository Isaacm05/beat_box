#ifndef SETUP_H
#define SETUP_H

#include "hardware/spi.h"
#include "lcd.h"
#include "pico/stdlib.h"

void setup_lcd();     // SPI + LCD init
void setup_spi_lcd(); // exposed separately if needed

#endif
