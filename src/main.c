#include "led_matrix.h"
#include "pico/stdlib.h"
#include "ui.h"
#include <stdio.h>

// Optional waveform generator support
// Uncomment when lcd and waveform-generator branches are merged:
// #define ENABLE_WAVEFORM_GEN

#ifdef ENABLE_WAVEFORM_GEN
#include "lcd/lcd.h"
#include "potentiometers/adc_potentiometer.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"

// Waveform generator variables
WaveParams adc_buffer;

// Auto-play timeout after editing
#define EDIT_TIMEOUT_MS 1000 // 1 sec of no edits
uint32_t last_edit_time = 0;
bool params_changed = false;

extern uint16_t pwm_buf[MAX_SAMPLES];
static float lcd_buf[MAX_SAMPLES]; // Float buffer for LCD display
#endif

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // Initialize LED matrix and UI
    led_matrix_init();
    ui_init();

#ifdef ENABLE_WAVEFORM_GEN
    // Initialize waveform generator and LCD
    pwm_audio_init();
    setup_lcd();
    waveform_init();

    adc_buffer = drum_presets[0];

    // Set the global pointer to our params (this is for later when we have 4 params)
    set_current_params(&adc_buffer);

    // Generate and display initial waveform
    waveform_generate(pwm_buf, MAX_SAMPLES, &adc_buffer);
    waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

    LCD_PrintWaveMenu(adc_buffer.waveform_id, (int) adc_buffer.frequency,
                      (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                      (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                      (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                      (int) (adc_buffer.comp_amount * 100), idx);
    LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
#endif

    // Main loop - integrated UI and waveform generation
    while (1) {
        // Update LED matrix UI
        led_matrix_refresh();
        ui_update();

#ifdef ENABLE_WAVEFORM_GEN
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Update waveform parameters from pots
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
            waveform_generate(pwm_buf, MAX_SAMPLES, &adc_buffer);
            waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

            // Redraw LCD display
            LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
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

        // Auto-play waveform after 1 second of no edits
        if (params_changed && (current_time - last_edit_time) >= EDIT_TIMEOUT_MS) {
            printf("Playing waveform...\n");
            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);
            params_changed = false;
        }

        // Sleep if nothing happened (save CPU)
        if (!params_updated) {
            sleep_ms(10);
        }
#endif
    }
}
