#include "led_matrix.h"
#include "ui.h"
#include "pico/stdlib.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    sleep_ms(2000);

    led_matrix_init();
    ui_init();

    while (1) {
        led_matrix_refresh();
        ui_update();
    }
}
