#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "lcd/lcd.h"
#include "lcd/lcd_setup.h"
#include "led/led_matrix.h"
#include "led/ui.h"
#include "pico/stdlib.h"
#include "potentiometers/adc_potentiometer.h"
#include "wavegen/presets.h"
#include "wavegen/pwm_audio.h"
#include "wavegen/waveform_gen.h"
#include <math.h>
#include <stdio.h>

WaveParams adc_buffer;

// Timing configuration for live editing
#define EDIT_TIMEOUT_MS 1000 // Play waveform after 1 second of no edits
uint32_t last_edit_time = 0;
bool params_changed = false;

extern uint16_t pwm_buf[MAX_SAMPLES];
static float lcd_buf[MAX_SAMPLES]; // Float buffer for LCD display

// Store WaveParams for each of the 4 tracks
#define NUM_TRACKS 4
static WaveParams track_params[NUM_TRACKS];

// Track the previously active track to detect changes
static int prev_active_track = -1;

int main() {
    stdio_init_all();
    printf("=== Live Waveform Editor ===\n");

    init_button(BUTTON_PIN_LEFT);
    init_button(BUTTON_PIN_RIGHT);
    init_adc_dma();

    led_matrix_init();
    ui_init();

    led_matrix_init();
    ui_init();
    pwm_audio_init();
    setup_lcd();

    // Initialize all tracks with preset 0 (Pure Sine - index 6)
    for (int i = 0; i < NUM_TRACKS; i++) {
        track_params[i] = drum_presets[6];
    }

    adc_buffer = drum_presets[0];

    // Set the global pointer to our params (this is for later when we have 8 params)
    set_current_params(&adc_buffer);

    // Generate and display initial waveform
    waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
    waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

    LCD_PrintWaveMenu(adc_buffer.waveform_id, (int) adc_buffer.frequency,
                      (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                      (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                      (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                      (int) (adc_buffer.comp_amount * 100), idx);
    LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);

    for (;;) {

        led_matrix_refresh();
        ui_update();

        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Get current active track and its preset
        int active_track = ui_get_active_track();
        int current_preset = ui_get_track_preset(active_track);

        // When active track changes, load that track's params into adc_buffer
        if (active_track != prev_active_track) {
            if (current_preset != -1) {
                // Load this track's stored params
                adc_buffer = track_params[active_track];

                // Regenerate waveforms
                waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
                waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

                // Update LCD display
                LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
                LCD_PrintWaveMenu(
                    adc_buffer.waveform_id, (int) adc_buffer.frequency,
                    (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                    (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                    (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                    (int) (adc_buffer.comp_amount * 100), idx);

                printf("Switched to track %d\n", active_track);
            }
            prev_active_track = active_track;
        }

        // When a preset is selected, load it into the track's params and adc_buffer
        if (current_preset != -1) {
            // Map LED UI preset index (0-3) to drum_presets array (6-9 for basic shapes)
            int preset_index = current_preset + 6;

            // Store the preset into this track's params (overwrites previous)
            track_params[active_track] = drum_presets[preset_index];

            // Also load into adc_buffer for immediate use
            adc_buffer = track_params[active_track];

            // Regenerate waveforms
            waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
            waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

            // Update LCD display
            LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
            LCD_PrintWaveMenu(
                adc_buffer.waveform_id, (int) adc_buffer.frequency,
                (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                (int) (adc_buffer.comp_amount * 100), idx);

            printf("Loaded preset %d into track %d\n", preset_index, active_track);
        }

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

            params_changed = false; // Reset change flag
        }

        // // only sleep if nothing happened
        // if (!params_updated) {
        //     sleep_ms(10); // 20 hz
        // }
    }
}
