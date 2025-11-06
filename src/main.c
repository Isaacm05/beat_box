#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.c"
#include <stdio.h>

#define BUTTON_PIN 14

volatile int current_preset = 0;
volatile bool play_flag = false;

void button_isr(uint gpio, uint32_t events) {
    current_preset = (current_preset + 1) % num_presets;
    play_flag = true;
}

int main() {
    stdio_init_all();
    printf("=== PWM Audio Playback Test ===\n");

    pwm_audio_init();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr);

    static float buffer[(int) SAMPLE_RATE];
    while (true) {
        if (play_flag) {
            play_flag = false;
            printf("Playing preset #%d\n", current_preset);

            // Generate waveform
            int n = waveform_generate(buffer, SAMPLE_RATE, &drum_presets[current_preset]);

            // Output through PWM
            pwm_play_buffer(buffer, n);
        }
        sleep_ms(10);
    }
}
