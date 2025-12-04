#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "lcd/lcd.h"
#include "lcd/lcd_setup.h"
#include "pico/stdlib.h"
#include "potentiometers/adc_potentiometer.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"
#include <math.h>
#include <stdio.h>

// // Memory optimized audio buffer: 16384 samples * 4 bytes = 64KB (was 176KB)
// static float buffer[MAX_SAMPLES];

// int current_preset = 0;
// volatile bool button_pressed = false;

// void test_button_isr() {
//     gpio_acknowledge_irq(21, GPIO_IRQ_EDGE_RISE);
//     button_pressed = true;
// }

// int main() {
//     stdio_init_all();
//     printf("=== PWM Audio Playback Test ===\n");

//     pwm_audio_init();
//     setup_lcd();

//     gpio_init(21);
//     gpio_pull_down(21);
//     gpio_add_raw_irq_handler_masked((1u << 21), test_button_isr);
//     irq_set_enabled(IO_IRQ_BANK0, true);
//     gpio_set_irq_enabled(21, GPIO_IRQ_EDGE_RISE, true);

//     for (;;) {
//         if (button_pressed) {
//             button_pressed = false;

//             printf("Playing preset #%d\n", current_preset);

//             // Generate waveform for current preset
//             waveform_generate(buffer, MAX_SAMPLES, &drum_presets[current_preset]);

//             printf("buffer[0] = %u\n", pwm_buf[0]);
//             printf("buffer[100] = %u\n", pwm_buf[100]);
//             printf("buffer[500] = %u\n", pwm_buf[500]);

//             // Only clear the waveform area instead of full screen (faster)
//             LCD_DrawFillRectangle(11, 60, 319, 239, BLACK);

//             // Start audio playback (non-blocking - returns immediately)
//             pwm_play_buffer_nonblocking(buffer, MAX_SAMPLES);

//             // Update LCD while audio plays in background
//             LCD_PlotWaveform(buffer, MAX_SAMPLES,
//                              (&drum_presets[current_preset])->waveform_id,
//                              100,
//                              100, 100, 100);

//             // Move to next preset for next button press
//             current_preset = (current_preset + 1) % 6;
//             printf("Done.\n");
//         }
//         sleep_ms(10);
//     }
//     return 0;
// }

// ============================================================================
// POTENTIOMETER-BASED LIVE EDITOR (CURRENT IMPLEMENTATION)
// ============================================================================
WaveParams adc_buffer;

// Timing configuration for live editing
#define EDIT_TIMEOUT_MS 1000 // Play waveform after 1 second of no edits
uint32_t last_edit_time = 0;
bool params_changed = false;

extern uint16_t pwm_buf[MAX_SAMPLES];
static float lcd_buf[MAX_SAMPLES]; // Float buffer for LCD display

int main() {
    stdio_init_all();
    printf("=== Live Waveform Editor ===\n");

    init_button(BUTTON_PIN_LEFT);
    init_button(BUTTON_PIN_RIGHT);
    init_adc_dma();

    pwm_audio_init();
    setup_lcd();

    adc_buffer = drum_presets[0];

    // Set the global pointer to our params (this is for later when we have 8 params)
    set_current_params(&adc_buffer);

    // Generate and display initial waveform
    LCD_PrintWaveMenu(0, (int) (0),
                      (int) (0), (int) (0),
                      (int) (0), (int) (0),
                      (int) (0), (int) (0),
                      (int) (0), 0);

    for (;;) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Update potentiometer values - returns true if params changed
        bool params_updated = update_pots(&adc_buffer);

        if (update_lcd_params) {
            update_lcd_params = false;
            
            LCD_PrintWaveMenu(
                adc_buffer.waveform_id, (int) adc_buffer.frequency,
                (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                (int) (adc_buffer.comp_amount * 100), idx);
        }
        if (params_updated || menu_updated) {
            // Regenerate waveform with new parameters
            waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
            waveform_generate(pwm_buf, MAX_SAMPLES, &adc_buffer); // Generate float version for

            // Redraw LCD display

            // LCD_DrawFillRectangle(11, 60, 319, 239, BLACK);

            LCD_PlotWaveform(pwm_buf, MAX_SAMPLES);
            LCD_PrintWaveMenu(
                adc_buffer.waveform_id, (int) adc_buffer.frequency,
                (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                (int) (adc_buffer.comp_amount * 100), idx);

            params_changed = true;
            menu_updated = false;
            last_edit_time = current_time;
        }

        if (params_changed && (current_time - last_edit_time) >= EDIT_TIMEOUT_MS) {
            printf("Playing waveform...\n");

            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);

            params_changed = false; // Reset change flag
        }

        // only sleep if nothing happened
        if (!params_updated) {
            sleep_ms(10); // 20 hz
        }
    }
}

// ============================================================================
// TEST MODE - SIMULATED POT CHANGES
// ============================================================================

// extern uint16_t pwm_buf[MAX_SAMPLES];

// int main() {
//     stdio_init_all();
//     printf("=== Waveform Animation Test (Simulated Pot) ===\n");

//     pwm_audio_init();
//     setup_lcd();

//     // Starting parameters
//     WaveParams test_params = {.frequency = 100.0f,
//                               .amplitude = 0.5f,
//                               .decay = 0.3f,
//                               .waveform_id = 0, // Sine wave
//                               .offset_dc = 0.0f,
//                               .pitch_decay = 0.0f,
//                               .noise_mix = 0.0f,
//                               .env_curve = 5.0f,
//                               .comp_amount = 0.0f};

//     printf("Sweeping frequency from 100 Hz to 2000 Hz\n");

//     // Sweep through frequencies to simulate pot turning
//     for (int i = 0; i < 40; i++) {
//         // Simulate pot movement - sweep frequency
//         test_params.frequency = 100.0f + (i * 50.0f); // 100 Hz to 2050 Hz
//         test_params.amplitude = 0.3f + (i * 0.015f);  // 0.3 to 0.9
//         test_params.decay = 0.2f + (i * 0.02f);       // 0.2 to 1.0s

//         printf("[%d/40] Freq: %.0f Hz, Amp: %.2f, Decay: %.2f s\n", i + 1, test_params.frequency,
//                test_params.amplitude, test_params.decay);

//         // Generate waveform
//         uint32_t gen_start = to_ms_since_boot(get_absolute_time());
//         waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &test_params);
//         uint32_t gen_end = to_ms_since_boot(get_absolute_time());

//         // Display on LCD (optimized - only clear waveform area, not full screen)
//         uint32_t lcd_start = to_ms_since_boot(get_absolute_time());
//         // Only clear the waveform area instead of full screen (huge speedup!)
//         // LCD_DrawFillRectangle(11, 60, 319, 239, 0x0000);
//         LCD_Clear(0x0000);
//         LCD_PlotWaveform(pwm_buf, MAX_SAMPLES, test_params.waveform_id, (int)
//         test_params.frequency,
//                          (int) (test_params.amplitude * 100), (int) (test_params.decay * 100),
//                          (int) (test_params.offset_dc * 100));
//         uint32_t lcd_end = to_ms_since_boot(get_absolute_time());

//         printf("  Gen: %lu ms, LCD: %lu ms, Total: %lu ms\n", gen_end - gen_start,
//                lcd_end - lcd_start, lcd_end - gen_start);

//         sleep_ms(50); // 20Hz update rate like pot version
//     }

//     printf("\nAnimation complete! Final waveform displayed.\n");
//     // Idle loop
//     for (;;) {
//         sleep_ms(1000);
//     }
// }