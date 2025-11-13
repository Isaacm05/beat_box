#include "pico/stdlib.h"
#include "Adafruit_Protomatter.h"

// ------------------------------------------------------------
// HUB75 PIN MAPPING (YOUR EXACT WIRING)
// ------------------------------------------------------------

// R1,G1,B1,R2,G2,B2
uint8_t rgbPins[] = {
    4,  // R1
    5,  // G1
    6,  // B1
    7,  // R2
    8,  // G2
    9   // B2
};

// A, B, C, D, E (5 address lines for 64px height)
uint8_t addrPins[] = {
    13, // A
    14, // B
    15, // C
    16, // D
    17  // E
};

// CLK = 11
// LAT = 12
// OE  = 10

// ------------------------------------------------------------
// CREATE PROTOMATTER OBJECT FOR 64x64 PANEL
// ------------------------------------------------------------

Adafruit_Protomatter matrix(
    64,            // width
    6,             // bit depth (1–6; 6 = full RGB565)
    1,             // number of parallel chains (usually 1)
    rgbPins,
    5,             // number of address lines (A–E → 5)
    addrPins,
    11,            // CLK
    12,            // LAT
    10,            // OE
    true           // double buffer enabled
);

// ------------------------------------------------------------
// MAIN PROGRAM
// ------------------------------------------------------------

int main() {
    stdio_init_all();
    sleep_ms(2000); // optional delay for USB terminal connection

    // Start the matrix driver
    ProtomatterStatus status = matrix.begin();
    if (status != PROTOMATTER_OK) {
        // Flash error code on LED if needed, but for now:
        while (true) {
            sleep_ms(200);
        }
    }

    // --------------------------------------------------------
    // Simple test loop: cycle colors
    // --------------------------------------------------------
    while (true) {

        matrix.fillScreen(matrix.color565(255, 0, 0));  // Red
        matrix.show();
        sleep_ms(1000);

        matrix.fillScreen(matrix.color565(0, 255, 0));  // Green
        matrix.show();
        sleep_ms(1000);

        matrix.fillScreen(matrix.color565(0, 0, 255));  // Blue
        matrix.show();
        sleep_ms(1000);

        // Example: draw some text if Adafruit_GFX is installed
        matrix.fillScreen(0);
        matrix.setCursor(0, 0);
        matrix.setTextColor(matrix.color565(255, 255, 0));
        matrix.setTextSize(1);
        matrix.print("HELLO RP2350!");
        matrix.show();
        sleep_ms(2000);
    }
}