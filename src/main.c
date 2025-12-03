#include "led_matrix.h"
#include "pico/stdlib.h"
#include "ui.h"
#include <stdio.h>

WaveParams adc_buffer;

// REMOVE THIS FOR MAIN, this is just for testing playing(or we can use idk)
#define EDIT_TIMEOUT_MS 1000 // 1 sec o fno edits
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

    for (;;) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

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
            waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer); // Generate float version for

            // Redraw LCD display

            // LCD_DrawFillRectangle(11, 60, 319, 239, BLACK);

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

        if (params_changed && (current_time - last_edit_time) >= EDIT_TIMEOUT_MS) {
            printf("Playing waveform...\n");

            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);

            params_changed = false; // Reset
        }

        // only sleep if nothing happened
        if (!params_updated) {
            sleep_ms(10); // 20 hz maybe we remove this?
        }
    }
}