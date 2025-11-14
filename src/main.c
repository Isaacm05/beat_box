#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"
#include "adc_potentiometer.h"
#include <stdio.h>

#define BUTTON_PIN 21

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int current_preset = 0;
bool play = false;

drum_preset = drum_presets[4]; // Example: select the "Tone" preset

static float buffer[(int) SAMPLE_RATE];
/*
void button_isr() {
    gpio_acknowledge_irq(BUTTON_PIN, GPIO_IRQ_EDGE_RISE);
    // printf("Hello World\n");

    printf("Playing preset #%d\n", current_preset);
    // Generate waveform
    waveform_generate(buffer, SAMPLE_RATE, &drum_presets[current_preset]);

    // Output through PWM
    current_preset = (current_preset + 1) % 6;

    play = true;
}
*/


int main() {
    /*stdio_init_all();
    printf("=== PWM Audio Playback Test ===\n");

    pwm_audio_init();
    setup_lcd();

    gpio_init(BUTTON_PIN);
    gpio_pull_down(BUTTON_PIN);
    gpio_add_raw_irq_handler_masked((1u << BUTTON_PIN), button_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true);

    for (;;) {
        if (play) {
            LCD_Clear(0x0000);

            pwm_play_buffer(buffer, (int) SAMPLE_RATE);
            LCD_PlotWaveform(buffer, 44100, 100, 100);
            play = false;
        }
        sleep_ms(10);
    }
    */

    stdio_init_all();

    init_button();
    init_adc_dma();

    pwm_audio_init();
    setup_lcd();

    adc_buffer= drum_preset; // Initialize pots to preset 4

    for(;;){
        // Main loop can be used to process adc_buffer based on mode_flag
        // For example, map adc_buffer values to parameters based on mode_flag

        check_pots();

        get_pots();

        normal_pots();

        drum_preset = adc_buffer; // Set drum_preset = pots vals
        waveform_generate(buffer, SAMPLE_RATE, &drum_preset);

        pwm_play_buffer(buffer, (int) SAMPLE_RATE);
        LCD_PlotWaveform(buffer, 44100, 100, 100);

        sleep_ms(500); // Adjust as needed
    }

}
