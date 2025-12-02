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
#define PIN_R2 5
#define PIN_G2 6
#define PIN_B2 7

// Row address pins
#define PIN_A 8
#define PIN_B 9
#define PIN_C 10
#define PIN_D 11
#define PIN_E 12

// Control pins
#define PIN_CLK 13
#define PIN_OE 14
#define PIN_LAT 15


#define COLOR_DEPTH 8

// Framebuffer (RGB888 format)
extern uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH][3];

void led_matrix_init(void);
void led_matrix_refresh(void);
void led_matrix_set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void led_matrix_clear(void);
void led_matrix_fill(uint8_t r, uint8_t g, uint8_t b);
void led_matrix_draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b);

#endif
