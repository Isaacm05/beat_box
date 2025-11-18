#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"            // Protomatter core API header (from library)
#include "arduino_compat.h"  // Your Arduino compatibility layer for RP2350

// Define your pins here (example pin numbers)
#define BIT_WIDTH   64
#define ADDR_COUNT  4   // 2^(ADDR_COUNT) * 2 = height, so 2^4*2=32 rows (adjust if 64 rows)
#define RGB_COUNT   1
#define BIT_DEPTH   6
#define TILE        1

// Example pins for your RGB, address, clock, latch, oe pins
uint8_t rgb_pins[6 * RGB_COUNT] = {
  2, 3, 4, 5, 6, 7  // R1, G1, B1, R2, G2, B2 pins (example)
};

uint8_t addr_pins[ADDR_COUNT] = {8, 9, 10, 11};  // row address pins

#define CLOCK_PIN 12
#define LATCH_PIN 13
#define OE_PIN    14

// Protomatter framebuffer size calculation:
#define ROWS (2 << ADDR_COUNT)  // height = 2 * 2^ADDR_COUNT
#define BUFFER_SIZE_BYTES ((BIT_WIDTH * ROWS * BIT_DEPTH) / 8)

// Framebuffer: raw 16-bit colors (RGB565) to be converted later
static uint16_t framebuffer[BIT_WIDTH * ROWS];

// Protomatter core struct
static Protomatter_core core;

void setPixel(int x, int y, uint16_t color) {
  if (x < 0 || x >= BIT_WIDTH || y < 0 || y >= ROWS) return;
  framebuffer[y * BIT_WIDTH + x] = color;
}

int main(void) {
  // Initialize your hardware, clocks, pins, etc.
  stdio_init_all();  // Pico standard IO init (optional)
  
  // Clear framebuffer
  memset(framebuffer, 0, sizeof(framebuffer));

  // Initialize Protomatter core
  ProtomatterStatus status = _PM_init(&core, BIT_WIDTH, BIT_DEPTH, RGB_COUNT, rgb_pins,
                                      ADDR_COUNT, addr_pins, CLOCK_PIN, LATCH_PIN, OE_PIN,
                                      false, TILE, NULL);
  if (status != PROTOMATTER_OK) {
    // Handle error (pins invalid, malloc fail, etc.)
    while (1);
  }

  // Start the display driver
  _PM_begin(&core);

  // Draw something: toggle a single row red full bright
  for (int x = 0; x < BIT_WIDTH; x++) {
    setPixel(x, 0, 0xF800);  // bright red in RGB565
  }

  while (1) {
    // Convert framebuffer from RGB565 to Protomatter internal format
    _PM_convert_565(&core, (uint8_t *)framebuffer, BIT_WIDTH);

    // Swap buffer if double-buffered; else just update
    _PM_swapbuffer_maybe(&core);

    // Small delay or your main loop code
    delay(16);  // ~60 FPS
  }

  return 0;
}
