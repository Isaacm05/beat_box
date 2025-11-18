#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>         // For bool, false in C
#include "core.h"            // Protomatter core API header (from library)
#include "arduino_compat.h"  // Your Arduino compatibility layer for RP2350

// Define your pins here (example pin numbers)
#define BIT_WIDTH   64
#define ADDR_COUNT  5   // 2^(ADDR_COUNT) * 2 = height, so 2^5*2=64 rows
#define RGB_COUNT   1
#define BIT_DEPTH   6
#define TILE        1

// Example pins for your RGB, address, clock, latch, oe pins
uint8_t rgb_pins[6 * RGB_COUNT] = {
  2, 3, 4, 5, 6, 7  // R1, G1, B1, R2, G2, B2 pins (example)
};

uint8_t addr_pins[ADDR_COUNT] = {8, 9, 10, 11, 12};  // row address pins

#define CLOCK_PIN 13
#define LATCH_PIN 14
#define OE_PIN    15

// Protomatter framebuffer size calculation:
// clearer formula for rows count
#define ROWS (2 * (1 << ADDR_COUNT))  // height = 2 * 2^ADDR_COUNT

// Framebuffer: raw 16-bit colors (RGB565) to be converted later
static uint16_t framebuffer[BIT_WIDTH * ROWS];

// Protomatter core struct
static Protomatter_core core;

void set_pixel(int x, int y, uint16_t color) {
  if (x < 0 || x >= BIT_WIDTH || y < 0 || y >= ROWS) return;
  framebuffer[y * BIT_WIDTH + x] = color;
}

int main(void) {
  stdio_init_all();  // Pico standard IO init (optional)
  
  memset(framebuffer, 0, sizeof(framebuffer));

  ProtomatterStatus status = _PM_init(&core, BIT_WIDTH, BIT_DEPTH, RGB_COUNT, rgb_pins,
                                      ADDR_COUNT, addr_pins, CLOCK_PIN, LATCH_PIN, OE_PIN,
                                      false, TILE, NULL);
  if (status != PROTOMATTER_OK) {
    while (1);  // Error handling: stuck here
  }

  _PM_begin(&core);

  // Set the top row to bright red
  for (int x = 0; x < BIT_WIDTH; x++) {
    set_pixel(x, 0, 0xF800);
  }

  while (1) {
    _PM_convert_565(&core, (uint8_t *)framebuffer, BIT_WIDTH);
    _PM_swapbuffer_maybe(&core);
    delay(16);  // ~60 FPS
  }

  return 0;  // Unreachable but good practice
}