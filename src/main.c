#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "lcd/lcd.h"
#include "lcd/lcd_setup.h"
#include "led/led_matrix.h"
#include "led/ui.h"
#include "pico/stdlib.h"
#include "potentiometers/adc_potentiometer.h"
#include "wavegen/mixer.h"
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
static bool track_has_preset[NUM_TRACKS] = {false, false, false, false};

// Track the previously active track to detect changes
static int prev_active_track = -1;
static bool ignore_pot_updates = false; // Flag to prevent pot overwrite on track switch

static uint16_t track_buffers[4][MAX_SAMPLES];
static int prev_beat_for_playback = -1;

// Callback function when a preset is selected on the LED UI
void on_preset_selected(int track, int preset) {
    // Preset index now maps directly to drum_presets array (0-9)
    int preset_index = preset;

    // Store the preset into this track's params (overwrites previous)
    track_params[track] = drum_presets[preset_index];
    track_has_preset[track] = true;

    // If this is the active track, update adc_buffer and LCD immediately
    int active_track = ui_get_active_track();
    if (track == active_track) {
        adc_buffer = track_params[track];

        // Regenerate waveforms
        waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
        waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer);

        // Update LCD display
        LCD_PlotWaveform(lcd_buf, MAX_SAMPLES);
        LCD_PrintWaveMenu(adc_buffer.waveform_id, (int) adc_buffer.frequency,
                          (int) (adc_buffer.amplitude * 100), (int) (adc_buffer.decay * 100),
                          (int) (adc_buffer.offset_dc * 100), (int) (adc_buffer.pitch_decay * 100),
                          (int) (adc_buffer.noise_mix * 100), (int) (adc_buffer.env_curve * 100),
                          (int) (adc_buffer.comp_amount * 100), idx);
    }

    printf("Loaded preset %d into track %d\n", preset_index, track);
}

int main() {
    stdio_init_all();
    printf("=== Live Waveform Editor ===\n");

    init_button(BUTTON_PIN_LEFT);
    init_button(BUTTON_PIN_RIGHT);
    init_adc_dma();

    led_matrix_init();
    ui_init();

    // Register preset callback
    ui_set_preset_callback(on_preset_selected);

    pwm_audio_init();
    setup_lcd();

    // Don't initialize tracks - they start empty until a preset is selected
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

        // Get current active track
        int active_track = ui_get_active_track();

        // When active track changes, load that track's params into adc_buffer (if it has a preset)
        if (active_track != prev_active_track) {

            if (track_has_preset[active_track]) {
                // Load this track's stored params
                adc_buffer = track_params[active_track];
                printf("New track params: freq=%.2f, amp=%.2f, decay=%.2f\n", adc_buffer.frequency,
                       adc_buffer.amplitude, adc_buffer.decay);
                // Regenerate waveforms
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
            } else {
                // Track has no preset - clear the LCD display
                LCD_Clear(BLACK);
                printf("Switched to track %d (no preset)\n", active_track);
            }
            prev_active_track = active_track;
            ignore_pot_updates = true; // Set flag to skip next pot update
        }

        // Update potentiometer values - returns true if params changed
        // Skip this update if we just switched tracks to avoid overwriting loaded params
        bool params_updated = false;
        if (!ignore_pot_updates) {
            params_updated = update_pots(&adc_buffer);
            track_params[active_track] = adc_buffer;

        } else {
            ignore_pot_updates = false; // Clear the flag after skipping one update
        }

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
            waveform_generate(lcd_buf, MAX_SAMPLES, &adc_buffer); // Generate float version for

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
            waveform_generate_pwm(pwm_buf, MAX_SAMPLES, &adc_buffer);
            pwm_play_pwm_nonblocking(pwm_buf, MAX_SAMPLES);

            params_changed = false; // Reset change flag
        }

        // // only sleep if nothing happened
        // if (!params_updated) {
        //     sleep_ms(10); // 20 hz
        // }

        // MIXER
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
                            waveform_generate(track_buffers[track], MAX_SAMPLES,
                                              &track_params[preset_id]);

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
    }
}
