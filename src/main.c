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
#include "wavegen/mixer.h"
#include <math.h>
#include <stdio.h>

WaveParams adc_buffer;

extern uint16_t pwm_buf[MAX_SAMPLES];
static float lcd_buf[MAX_SAMPLES]; // Float buffer for LCD display

// Track buffers for mixing
static uint16_t track_buffers[4][MAX_SAMPLES];

#define LONG_PRESS_MS 500
static uint32_t button_press_time_left = 0;
static uint32_t button_press_time_right = 0;
static bool button_left_pressed = false;
static bool button_right_pressed = false;
static bool button_left_long_handled = false;
static bool button_right_long_handled = false;

// Track previous active track to detect changes
static int prev_active_track = -1;
static int prev_beat_for_playback = -1;

void check_button_long_press(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Check left button state
    bool left_state = (gpio_get(BUTTON_PIN_LEFT) == 0); // Active low
    if (left_state && !button_left_pressed) {
        button_left_pressed = true;
        button_press_time_left = current_time;
        button_left_long_handled = false;
    } else if (!left_state && button_left_pressed) {
        button_left_pressed = false;
    }

    // Check right button state
    bool right_state = (gpio_get(BUTTON_PIN_RIGHT) == 0); // Active low
    if (right_state && !button_right_pressed) {
        button_right_pressed = true;
        button_press_time_right = current_time;
        button_right_long_handled = false;
    } else if (!right_state && button_right_pressed) {
        button_right_pressed = false;
    }

    // Handle long press for left button
    if (button_left_pressed && !button_left_long_handled &&
        (current_time - button_press_time_left) >= LONG_PRESS_MS) {
        button_left_long_handled = true;

        // Play sample preview
        int active_track = ui_get_active_track();
        int preset_id = ui_get_track_preset(active_track);
        if (preset_id >= 0 && preset_id < num_presets) {
            waveform_generate(pwm_buf, MAX_SAMPLES, &drum_presets[preset_id]);
            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);
            printf("Preview track %d preset %d\n", active_track, preset_id);
        }
    }

    // Handle long press for right button
    if (button_right_pressed && !button_right_long_handled &&
        (current_time - button_press_time_right) >= LONG_PRESS_MS) {
        button_right_long_handled = true;

        // Play sample preview
        int active_track = ui_get_active_track();
        int preset_id = ui_get_track_preset(active_track);
        if (preset_id >= 0 && preset_id < num_presets) {
            waveform_generate(pwm_buf, MAX_SAMPLES, &drum_presets[preset_id]);
            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);
            printf("Preview track %d preset %d\n", active_track, preset_id);
        }
    }
}

int main() {
    stdio_init_all();

    // Initialize all peripherals
    init_button(BUTTON_PIN_LEFT);
    init_button(BUTTON_PIN_RIGHT);
    init_adc_dma();

    pwm_audio_init();
    setup_lcd();
    waveform_init();
    mixer_init();

    // LED M
    led_matrix_init();
    ui_init();

    // Start with first preset as default
    adc_buffer = drum_presets[0];
    set_current_params(&adc_buffer);

    // Generate and display initial waveform on LCD
    waveform_generate(pwm_buf, MAX_SAMPLES, &adc_buffer);
    waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

    LCD_PrintWaveMenu(adc_buffer.waveform_id, (int) adc_buffer.frequency,
                      (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                      (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                      (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                      (int) (adc_buffer.comp_amount * 100), idx);
    LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);

    for (;;) {
        //LED
        ui_update();
        led_matrix_refresh();

        // Check for button long press on LCD buttons
        check_button_long_press();

        // get active track
        int active_track = ui_get_active_track();

        // if it changed update lcd
        if (active_track != prev_active_track) {
            prev_active_track = active_track;

            int preset_id = ui_get_track_preset(active_track);
            if (preset_id >= 0 && preset_id < num_presets) {
                // adc_buffer with the active track's preset
                adc_buffer = drum_presets[preset_id];
                set_current_params(&adc_buffer);

                // Regenerate waveform
                waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

                // Update LCD display
                LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
                LCD_PrintWaveMenu(
                    adc_buffer.waveform_id, (int) adc_buffer.frequency,
                    (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                    (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                    (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                    (int) (adc_buffer.comp_amount * 100), idx);

                printf("Active track changed to %d with preset %d\n", active_track, preset_id);
            }
        }

        // Update potentiometers for active track's waveform
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

            // Update LCD display
            LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
            LCD_PrintWaveMenu(
                adc_buffer.waveform_id, (int) adc_buffer.frequency,
                (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                (int) (adc_buffer.comp_amount * 100), idx);

            menu_updated = false;

            // Update the preset in drum_presets array with modified parameters
            int preset_id = ui_get_track_preset(active_track);
            if (preset_id >= 0 && preset_id < num_presets) {
                drum_presets[preset_id] = adc_buffer;
            }
        }

        // Handle beat playback when playing
        if (ui_is_playing()) {
            int current_beat = ui_get_current_beat();

            // Only trigger on beat changes to avoid retriggering
            if (current_beat != prev_beat_for_playback) {
                prev_beat_for_playback = current_beat;

                // Clear mixer for new beat
                mixer_clear();

                // Check all tracks for active beats at current position
                for (int track = 0; track < 4; track++) {
                    if (ui_get_beat_state(track, current_beat)) {
                        int preset_id = ui_get_track_preset(track);
                        if (preset_id >= 0 && preset_id < num_presets) {
                            // Generate waveform for this track
                            waveform_generate(track_buffers[track], MAX_SAMPLES, &drum_presets[preset_id]);

                            // Add track to mixer
                            mixer_add_track(track, track_buffers[track], MAX_SAMPLES);
                            printf("Added track %d to mix at beat %d\n", track, current_beat);
                        }
                    }
                }

                // If any tracks were added, play the mixed output
                if (mixer_has_active_tracks()) {
                    int mix_len;
                    const uint16_t* mixed_output = mixer_get_output(&mix_len);
                    if (mixed_output) {
                        pwm_play_pwm_nonblocking(mixed_output, mix_len);
                        printf("Playing mixed output (%d samples)\n", mix_len);
                    }
                }
            }
        } else {
            if (prev_beat_for_playback != -1) {
                pwm_stop_audio();
                printf("Playback stopped\n");
            }
            prev_beat_for_playback = -1;
        }

        sleep_ms(1);
    }
}
