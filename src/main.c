#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"
#include <stdio.h>

#define BUTTON_PIN 21

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int current_preset = 0;
bool play = false;

static float buffer[(int) SAMPLE_RATE];
// void print_buffer(float* buf, int length) {
//     for (int i = 0; i < length; i++) {
//         printf("%f ", buf[i]);
//         if ((i + 1) % 10 == 0)
//             printf("\n"); // new line every 10 values
//     }
//     printf("\n");
// }

void button_isr() {
    gpio_acknowledge_irq(BUTTON_PIN, GPIO_IRQ_EDGE_RISE);
    // printf("Hello World\n");

    printf("Playing preset #%d\n", current_preset);
    // Generate waveform
    waveform_generate(buffer, SAMPLE_RATE, &drum_presets[current_preset]);
    // Output through PWM

    // for (int i = 0; i < SAMPLE_RATE; i++) {
    //     buffer[i] = 0.5f * sinf(2 * M_PI * 440.0f * i / SAMPLE_RATE);
    // }
    current_preset = (current_preset + 1) % 6;

    play = true;
}

int main() {
    stdio_init_all();
    printf("=== PWM Audio Playback Test ===\n");

    pwm_audio_init();

    gpio_init(BUTTON_PIN);
    gpio_pull_down(BUTTON_PIN);
    gpio_add_raw_irq_handler_masked((1u << BUTTON_PIN), button_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true);
    for (;;) {
        if (play) {
            pwm_play_buffer(buffer, (int) SAMPLE_RATE);
            play = false;
        }
        sleep_ms(10);
    }
}
