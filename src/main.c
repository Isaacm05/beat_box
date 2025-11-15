#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "lcd/lcd.h"
#include "lcd/lcd_setup.h"
#include "lcd/lcd_waveform_fast.h"
#include "pico/stdlib.h"
#include "potentiometers/adc_potentiometer.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"
#include <stdio.h>

// Memory optimized audio buffer: 16384 samples * 4 bytes = 64KB (was 176KB)
static float buffer[MAX_SAMPLES];

int current_preset = 0;
volatile bool button_pressed = false;

void test_button_isr() {
    gpio_acknowledge_irq(21, GPIO_IRQ_EDGE_RISE);
    button_pressed = true;
}

int main() {
    stdio_init_all();
    printf("=== PWM Audio Playback Test ===\n");

    pwm_audio_init();
    setup_lcd();

    gpio_init(21);
    gpio_pull_down(21);
    gpio_add_raw_irq_handler_masked((1u << 21), test_button_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
    gpio_set_irq_enabled(21, GPIO_IRQ_EDGE_RISE, true);

    for (;;) {
        if (button_pressed) {
            button_pressed = false;

            printf("Playing preset #%d\n", current_preset);

            // Generate waveform for current preset
            waveform_generate(buffer, MAX_SAMPLES, &drum_presets[current_preset]);

            printf("buffer[0] = %u\n", pwm_buf[0]);
            printf("buffer[100] = %u\n", pwm_buf[100]);
            printf("buffer[500] = %u\n", pwm_buf[500]);

            // Only clear the waveform area instead of full screen (faster)
            LCD_DrawFillRectangle(11, 60, 319, 239, BLACK);

            // Start audio playback (non-blocking - returns immediately)
            pwm_play_buffer_nonblocking(buffer, MAX_SAMPLES);

            // Update LCD while audio plays in background
            LCD_PlotWaveform(buffer, MAX_SAMPLES, (&drum_presets[current_preset])->waveform_id, 100,
                             100, 100, 100);

            // Move to next preset for next button press
            current_preset = (current_preset + 1) % 6;
            printf("Done.\n");
        }
        sleep_ms(10);
    }
    return 0;
}

// WaveParams adc_buffer;

// int main() {
//     stdio_init_all();

//     init_button();
//     init_adc_dma();

//     pwm_audio_init();
//     setup_lcd();

//     for (;;) {
//         // Main loop can be used to process adc_buffer based on mode_flag
//         // For example, map adc_buffer values to parameters based on mode_flag

//         get_pot_val();
//         adc_buffer = normal_pots(adc_buffer);

//         waveform_generate(buffer, SAMPLE_RATE, &adc_buffer);

//         pwm_play_buffer(buffer, (int) SAMPLE_RATE);
//         LCD_PlotWaveform(buffer, 44100, adc_buffer.waveform_id, (int) adc_buffer.frequency,
//                          (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
//                          (int) (adc_buffer.offset_dc * 100));

//         sleep_ms(500); // Adjust as needed
//     }
// }
