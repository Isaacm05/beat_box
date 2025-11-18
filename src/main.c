#include <stdio.h>
#include "pico/stdlib.h"
#include "Adafruit_Protomatter.h"    // The Protomatter core and rp2040 backend header

// Include arduino_compat.h if not included in rp2040.h already
// #include "arduino_compat.h" 

// Panel configuration macros for 64x64 HUB75 panel, 1-bit depth (for testing)
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 64
#define MATRIX_DEPTH 1    // Use 1 bitplane for simplicity in testing

// Define your pin assignments here for the HUB75 signals (change as needed)
// These pins must match your wiring to the panel
// Pins must be GPIO numbers as per Pico SDK

static const uint8_t R1_pin = 1;
static const uint8_t G1_pin = 2;
static const uint8_t B1_pin = 3;
static const uint8_t R2_pin = 4;
static const uint8_t G2_pin = 6;
static const uint8_t B2_pin = 7;
static const uint8_t A_pin  = 12;
static const uint8_t B_pin  = 13;
static const uint8_t C_pin  = 14;
static const uint8_t D_pin  = 16;
static const uint8_t E_pin  = 17;
static const uint8_t LAT_pin = 11;
static const uint8_t OE_pin  = 8;
static const uint8_t CLK_pin = 9;

// Declare the pins array for Protomatter:
// The order is important and depends on your panel and Protomatter expectations.
// Protomatter expects 16 pins:
// R1,G1,B1,R2,G2,B2,A,B,C,D,LAT,OE,CLK, and 3 more unused pins (set to 255 if unused)
// this one uses an E pin because its 64x64

static const uint8_t pins[] = {
    R1_pin, G1_pin, B1_pin,
    R2_pin, G2_pin, B2_pin,
    A_pin, B_pin, C_pin, D_pin, E_pin,
    LAT_pin, OE_pin, CLK_pin,
    255, 255 // Unused pins
};

// Declare the Protomatter core struct globally
static Protomatter_core matrix_core;

int main() {
    // Initialize stdio so you can see debug output on USB serial (optional)
    stdio_init_all();
    sleep_ms(500); // wait a moment for serial connection

    printf("Starting Protomatter RP2350 test on 64x64 panel\n");

    // Initialize the matrix_core with parameters:
    //   &matrix_core,   pointer to Protomatter core struct
    //   MATRIX_WIDTH,   width of the panel (64)
    //   MATRIX_HEIGHT,  height of the panel (64)
    //   MATRIX_DEPTH,   bit depth (1 bitplane here)
    //   pins,           pointer to the pin array
    //   13              number of pins in array (excluding 255 unused)
    Protomatter_begin(&matrix_core, MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_DEPTH, pins, 13);

    // Set the global pointer for the RP2040 backend ISR
    _PM_protoPtr = &matrix_core;

    // Clear the panel buffer (all pixels off)
    Protomatter_clear(&matrix_core);

    // Main loop: toggle row 0 pixels on and off every 500ms
    while (true) {
        // Turn on row 0, all columns to white (max color for 1 bitplane)
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            Protomatter_drawPixel(&matrix_core, x, 0, 0xFFFF); // white color for 1 bitplane
        }
        Protomatter_update(&matrix_core);  // Push buffer to hardware

        sleep_ms(500);

        // Turn off row 0
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            Protomatter_drawPixel(&matrix_core, x, 0, 0x0000); // black/off
        }
        Protomatter_update(&matrix_core);

        sleep_ms(500);
    }

    return 0;
}
