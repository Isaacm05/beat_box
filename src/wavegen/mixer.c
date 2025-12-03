#include "mixer.h"
#include "pwm_audio.h"
#include <stdio.h>
#include <string.h>

// Mix buffer for combining multiple audio tracks
static uint32_t mix_buffer[MAX_SAMPLES];    // Use 32-bit for intermediate mixing
static uint16_t output_buffer[MAX_SAMPLES]; // Final output buffer
static int mix_length = 0;
static int active_track_count = 0;

void mixer_init(void) {
    mixer_clear();
}

void mixer_add_track(int track_id, const uint16_t* buffer, int len) {
    if (!buffer || len <= 0 || len > MAX_SAMPLES) {
        return;
    }

    // If this is the first track, initialize the length
    if (active_track_count == 0) {
        mix_length = len;
        memset(mix_buffer, 0, sizeof(mix_buffer));
    } else {
        // Use the maximum length if tracks differ
        if (len > mix_length) {
            mix_length = len;
        }
    }

    for (int i = 0; i < len; i++) {
        mix_buffer[i] += buffer[i];
    }

    active_track_count++;
}

const uint16_t* mixer_get_output(int* len) {
    if (active_track_count == 0) {
        *len = 0;
        return NULL;
    }

    for (int i = 0; i < mix_length; i++) {
        uint32_t mixed_sample = mix_buffer[i];

        // keep amplitude
        mixed_sample = mixed_sample / active_track_count;

        // Clip 
        if (mixed_sample > 255) {
            mixed_sample = 255;
        }

        output_buffer[i] = (uint16_t) mixed_sample;
    }

    *len = mix_length;

    return output_buffer;
}

void mixer_clear(void) {
    memset(mix_buffer, 0, sizeof(mix_buffer));
    mix_length = 0;
    active_track_count = 0;
}

bool mixer_has_active_tracks(void) {
    return active_track_count > 0;
}
