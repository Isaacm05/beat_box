#include "stdlib.h"
#include "core.h"

Protomatter matrix;

void frameISR(void *ptr) {
    _PM_row_handler((Protomatter *)ptr);
}

void initMatrix() {

    const uint8_t rgbPins[] = {4, 5, 6, 7, 8, 9};        // R1 G1 B1 R2 G2 B2
    const uint8_t addrPins[] = {13, 14, 15, 16, 17};     // A B C D E (1/32 scan)

    protomatter_init(
        &matrix,
        64,       // width
        4,        // bit depth
        1,        // chains
        rgbPins, 6,
        11,       // CLK
        12,       // LAT
        10,       // OE
        addrPins, 5
    );

    protomatter_start(&matrix, frameISR);
}

int main() {
    stdio_init_all();
    initMatrix();

    int y = 0;

    while (1) {
        for (int i = 0; i < 64 * 64; i++)
            matrix.framebuffer[i] = 0;

        for (int x = 0; x < 64; x++)
            matrix.framebuffer[y * 64 + x] = 0xFFFF;

        y = (y + 1) % 64;

        sleep_ms(20);
    }
}