#ifndef BB_BACKEND_H
#define BB_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "waveform_gen.h"  // WaveParams
#include "pwm_audio.h"     // SAMPLE_RATE, MAX_SAMPLES

/*
Config, note that stuff here may be adjusted later, especially since its based
off the visual aid we made before
*/ 
#define BB_MAX_TRACKS 4
#define BB_NUM_STEPS  16
#define BB_NUM_PARAMS 8

//Types

typedef struct {
    bool       used;                  // track active?
    char       name[12];              // Track 1, Kick, etc.
    WaveParams params;                // synth parameters for this track
    uint16_t   pattern;               // 16-bit step mask (1 bit per step)
} BbTrack;

typedef struct {
    // Sequencer state
    bool     playing;
    uint8_t  current_step;            // 0..15
    uint16_t bpm;                     // beats per minute
    uint16_t ms_per_step;             // derived from bpm

    // UI selection (LED + TFT based)
    uint8_t  selected_track;          // 0..BB_MAX_TRACKS-1
    uint8_t  selected_step;           // 0..15 (for LED/grid editing)
    uint8_t  selected_param;          // 0..7 (for TFT param editing)
} BbState;


// Initialize backend, tracks, defaults
void bb_init(void);

// Sequencer / timebase (for 64x64 LED + audio)


// Call from a 1 kHz timer or SysTick (1 ms tick)
void bb_tick_1ms(void);

void bb_play(void);
void bb_stop(void);
void bb_toggle_play(void);
void bb_set_bpm(uint16_t bpm);

// Track + step editing (for LED grid / buttons)

// Move selection between tracks (skips unused tracks)
void bb_next_track(void);
void bb_prev_track(void);

// Move selection between steps (0..15)
void bb_next_step(void);
void bb_prev_step(void);

// Toggle a specific step in a track (pattern bit flip)
void bb_toggle_step(uint8_t track_idx, uint8_t step);

// Toggle currently selected cell 
//(Idk if we need this or not, cause we havent discussed being able to skip steps to toggle on or off)
// but technically its used in like almost all DAWs
void bb_toggle_selected_step(void);

// Track management: max 4, min 1
void bb_add_track(void);
void bb_delete_selected_track(void);


// ***TFT stuff, may need to change this as code is not pushed for the TFT***

// Cycle which WaveParams field is selected (0..7)
void bb_next_param(void);
void bb_prev_param(void);

// Map ADC reading from single pot (0..4095) into the selected parameter.
// You should call this whenever the ADC value changes while you are in
// "waveform detail" mode on the TFT.
void bb_param_from_adc(uint16_t adc_raw);

// -----------------------------------------------------------------------------
// Getters for UI (LED + TFT)
// -----------------------------------------------------------------------------

// Global state (do NOT modify the returned pointer)
const BbState *bb_get_state(void);

// Array of 4 tracks (do not modify directly outside backend if you can avoid it)
const BbTrack *bb_get_tracks(void);

// Currently selected track's WaveParams (for TFT)
const WaveParams *bb_get_selected_wave(void);

// Generate a preview waveform for the TFT using the selected track's parameters.
// Returns number of samples written (<= max_samples).
int bb_generate_preview(float *buffer, int max_samples);

#endif
