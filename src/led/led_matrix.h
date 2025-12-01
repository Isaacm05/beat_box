// led_matrix.h
#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

// Panel configuration
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 64
#define SCAN_ROWS 32 // 64 / 2, panel is 1/32 scan

// RGB Data pins - Upper half (directly consecutive for PIO)
#define PIN_R1 2
#define PIN_G1 3
#define PIN_B1 4

// RGB Data pins - Lower half
#define PIN_R2 6
#define PIN_G2 7
#define PIN_B2 8

// Row address pins
#define PIN_A 9
#define PIN_B 11
#define PIN_C 12
#define PIN_D 13
#define PIN_E 14

// Control pins
#define PIN_CLK 16
#define PIN_OE 17  
#define PIN_LAT 18 


#define COLOR_DEPTH 8

// Framebuffer (RGB888 format)
extern uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];

void led_matrix_init(void);
void led_matrix_refresh(void);
void led_matrix_set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void led_matrix_clear(void);
void led_matrix_fill(uint8_t r, uint8_t g, uint8_t b);

#endif