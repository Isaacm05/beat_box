// beatbox_backend.c
#include "bb_backend.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "waveform_gen.h"
#include "pwm_audio.h"
#include "presets.h"


static BbTrack g_tracks[BB_MAX_TRACKS];
static BbState g_state;

// Shared audio buffer for per-step hits / TFT preview
static float g_hit_buf[MAX_SAMPLES];

// Helper: number of presets in presets.h
static int bb_num_presets(void) {
    return (int)(sizeof(drum_presets) / sizeof(WaveParams));
}

// helpers

static void bb_update_ms_per_step(void) {
    if (g_state.bpm == 0) g_state.bpm = 1;
    // 16 steps = 4 beats (4/4) => 4 steps per beat
    uint32_t ms_per_beat = 60000u / g_state.bpm;
    g_state.ms_per_step = (uint16_t)(ms_per_beat / 4);
}

static void bb_init_tracks_from_presets(void) {
    int n_presets = bb_num_presets();

    for (int i = 0; i < BB_MAX_TRACKS; i++) {
        g_tracks[i].used    = false;
        g_tracks[i].pattern = 0;
        snprintf(g_tracks[i].name, sizeof(g_tracks[i].name),
                 "Track %d", i + 1);

        // If we have enough presets, seed each track from a different one
        if (i < n_presets) {
            g_tracks[i].params = drum_presets[i];
        } else if (n_presets > 0) {
            g_tracks[i].params = drum_presets[0];
        } else {
            // Fallback defaults
            g_tracks[i].params.frequency   = 200.0f;
            g_tracks[i].params.amplitude   = 0.8f;
            g_tracks[i].params.decay       = 0.4f;
            g_tracks[i].params.waveform_id = 0;
            g_tracks[i].params.offset_dc   = 0.0f;
            g_tracks[i].params.pitch_decay = 3.0f;
            g_tracks[i].params.noise_mix   = 0.0f;
            g_tracks[i].params.env_curve   = 3.0f;
            g_tracks[i].params.comp_amount = 0.0f;
        }
    }

    // Start with Track 0 active, others inactive
    g_tracks[0].used = true;

    // Nice default pattern on Track 0 (4-on-the-floor)
    g_tracks[0].pattern = 0b1000100010001000;
}

// Trigger one track at current step (generate + play)
// Simple version: only fire if PWM is idle (no mixing yet).
static void bb_trigger_track(int track_idx) {
    if (track_idx < 0 || track_idx >= BB_MAX_TRACKS) return;
    if (!g_tracks[track_idx].used) return;

    // Avoid overlapping DMA for now
    if (pwm_is_playing()) return;

    int n = waveform_generate(g_hit_buf, MAX_SAMPLES, &g_tracks[track_idx].params);
    if (n <= 0) return;

    pwm_play_buffer_nonblocking(g_hit_buf, n);
}

// Find the next used track index when moving +1/-1 from current selection
static int bb_find_next_used_track(int start, int dir) {
    // dir = +1 or -1
    for (int i = 1; i <= BB_MAX_TRACKS; i++) {
        int idx = (start + dir * i + BB_MAX_TRACKS) % BB_MAX_TRACKS;
        if (g_tracks[idx].used) return idx;
    }
    // If no used tracks (should never happen because we enforce min 1),
    // just stay on the same index.
    return start;
}

// Count how many tracks are currently used
static int bb_count_used_tracks(void) {
    int count = 0;
    for (int i = 0; i < BB_MAX_TRACKS; i++) {
        if (g_tracks[i].used) count++;
    }
    return count;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void bb_init(void) {
    memset(&g_state, 0, sizeof(g_state));

    g_state.playing        = false;
    g_state.current_step   = 0;
    g_state.bpm            = 120;  // default tempo
    g_state.selected_track = 0;
    g_state.selected_step  = 0;
    g_state.selected_param = 0;

    bb_update_ms_per_step();
    bb_init_tracks_from_presets();
}

void bb_tick_1ms(void) {
    static uint32_t ms_accum = 0;

    if (!g_state.playing) return;

    ms_accum++;
    if (ms_accum < g_state.ms_per_step) {
        return;
    }
    ms_accum = 0;

    uint8_t step = g_state.current_step;

    // For now: trigger the FIRST track that has this step set.
    // (You can upgrade this to mixing later if you want.)
    for (int t = 0; t < BB_MAX_TRACKS; t++) {
        if (!g_tracks[t].used) continue;
        if (!((g_tracks[t].pattern >> step) & 1u)) continue;

        bb_trigger_track(t);
        break;  // one hit per step for now
    }

    g_state.current_step = (uint8_t)((step + 1) % BB_NUM_STEPS);
}

void bb_play(void) {
    g_state.playing      = true;
    g_state.current_step = 0;
}

void bb_stop(void) {
    g_state.playing = false;
}

void bb_toggle_play(void) {
    if (g_state.playing) bb_stop();
    else                 bb_play();
}

void bb_set_bpm(uint16_t bpm) {
    if (bpm == 0) bpm = 1;
    g_state.bpm = bpm;
    bb_update_ms_per_step();
}

// Track + step editing (for LED/grid)


void bb_next_track(void) {
    g_state.selected_track = (uint8_t)bb_find_next_used_track(
        g_state.selected_track, +1);
}

void bb_prev_track(void) {
    g_state.selected_track = (uint8_t)bb_find_next_used_track(
        g_state.selected_track, -1);
}

void bb_next_step(void) {
    g_state.selected_step =
        (uint8_t)((g_state.selected_step + 1) % BB_NUM_STEPS);
}

void bb_prev_step(void) {
    g_state.selected_step =
        (uint8_t)((g_state.selected_step + BB_NUM_STEPS - 1) % BB_NUM_STEPS);
}

void bb_toggle_step(uint8_t track_idx, uint8_t step) {
    if (track_idx >= BB_MAX_TRACKS) return;
    if (step >= BB_NUM_STEPS) return;
    if (!g_tracks[track_idx].used) return;

    uint16_t mask = (1u << step);
    g_tracks[track_idx].pattern ^= mask;
}

void bb_toggle_selected_step(void) {
    bb_toggle_step(g_state.selected_track, g_state.selected_step);
}

// Track management: max 4, min 1

void bb_add_track(void) {
    // Find first unused slot
    int slot = -1;
    for (int i = 0; i < BB_MAX_TRACKS; i++) {
        if (!g_tracks[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return; // already at max

    g_tracks[slot].used = true;
    snprintf(g_tracks[slot].name, sizeof(g_tracks[slot].name),
             "Track %d", slot + 1);

    int n_presets = bb_num_presets();
    if (n_presets > 0) {
        // use a preset, wrap around if needed
        int preset_idx = slot % n_presets;
        g_tracks[slot].params = drum_presets[preset_idx];
    }

    g_tracks[slot].pattern = 0; // empty pattern initially
    g_state.selected_track = (uint8_t)slot;
}

void bb_delete_selected_track(void) {
    int used = bb_count_used_tracks();
    if (used <= 1) {
        // keep at least one track
        return;
    }

    int idx = g_state.selected_track;
    g_tracks[idx].used    = false;
    g_tracks[idx].pattern = 0;

    // Move selection to nearest used track
    g_state.selected_track = (uint8_t)bb_find_next_used_track(idx, +1);
}

// -----------------------------------------------------------------------------
// Parameter editing (TFT + pot)
// -----------------------------------------------------------------------------

void bb_next_param(void) {
    g_state.selected_param++;
    if (g_state.selected_param >= BB_NUM_PARAMS) {
        g_state.selected_param = 0;
    }
}

void bb_prev_param(void) {
    if (g_state.selected_param == 0) {
        g_state.selected_param = BB_NUM_PARAMS - 1;
    } else {
        g_state.selected_param--;
    }
}

void bb_param_from_adc(uint16_t adc_raw) {
    // adc_raw: 0..4095 -> t: 0.0..1.0
    if (adc_raw > 4095) adc_raw = 4095;
    float t = (float)adc_raw / 4095.0f;

    BbTrack *tr = &g_tracks[g_state.selected_track];
    WaveParams *p = &tr->params;

    switch (g_state.selected_param) {
    case 0: // frequency 20–9000 Hz
        p->frequency = 20.0f + t * (9000.0f - 20.0f);
        break;
    case 1: // amplitude 0–1
        p->amplitude = t;
        break;
    case 2: // decay 0–2.0 s
        p->decay = t * 2.0f;
        break;
    case 3: // DC offset 0–1
        p->offset_dc = t;
        break;
    case 4: // pitch decay 0–10
        p->pitch_decay = t * 10.0f;
        break;
    case 5: // noise mix 0–1
        p->noise_mix = t;
        break;
    case 6: // env curve 0–10
        p->env_curve = t * 10.0f;
        break;
    case 7: // compression amount 0–1
        p->comp_amount = t;
        break;
    default:
        break;
    }
}

// Getters

const BbState *bb_get_state(void) {
    return &g_state;
}

const BbTrack *bb_get_tracks(void) {
    return g_tracks;
}

const WaveParams *bb_get_selected_wave(void) {
    // If selected track is unused (shouldn't happen), fall back to first used.
    if (!g_tracks[g_state.selected_track].used) {
        for (int i = 0; i < BB_MAX_TRACKS; i++) {
            if (g_tracks[i].used) {
                return &g_tracks[i].params;
            }
        }
    }
    return &g_tracks[g_state.selected_track].params;
}

int bb_generate_preview(float *buffer, int max_samples) {
    if (!buffer || max_samples <= 0) return 0;
    BbTrack *tr = &g_tracks[g_state.selected_track];
    return waveform_generate(buffer, max_samples, &tr->params);
}
