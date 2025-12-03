#include "waveform_gen.h"
#include "pwm_audio.h"
#include <math.h>
#include <stdlib.h>

#define M_PI 3.14159265358979323846

#define SINE_TABLE_SIZE 256
static float sine_table[SINE_TABLE_SIZE];

void waveform_init(void) {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        sine_table[i] = sinf(2.0f * M_PI * i / SINE_TABLE_SIZE);
    }
}

static inline float fast_sin(float phase) {
    phase = phase - (int) phase;
    if (phase < 0.0f) {
        phase += 1.0f;
    }

    int index = (int) (phase * SINE_TABLE_SIZE) & (SINE_TABLE_SIZE - 1);
    return sine_table[index];
}

static void compute_envelope_cache(float* envelope_cache, int total_samples,
                                   float env_decay_factor) {
    for (int i = 0; i < total_samples; i++) {
        envelope_cache[i] = expf(env_decay_factor * i);
    }
}

static void compute_pitch_decay_cache(float* pitch_cache, int total_samples,
                                      float pitch_decay_factor) {
    for (int i = 0; i < total_samples; i++) {
        pitch_cache[i] = expf(pitch_decay_factor * i);
    }
}

// Waveforms
static inline float wave_sine(float phase) {
    return sinf(2 * M_PI * phase);
}
static inline float wave_square(float phase) {
    return (phase < 0.5f) ? 1.0f : -1.0f;
}
static inline float wave_triangle(float phase) {
    return 4.0f * fabsf(phase - 0.5f) - 1.0f;
}
static inline float wave_saw(float phase) {
    return 2.0f * phase - 1.0f;
}
static inline float wave_noise() {
    return (rand() / (float) RAND_MAX) * 2.0f - 1.0f;
}

// USE THIS ONE
int waveform_generate(uint16_t* pwm_buffer, int max_samples, WaveParams* p) {

    // Calculate timing and sample count
    float dt = 1.0f / SAMPLE_RATE;
    int total_samples = (int) (p->decay * SAMPLE_RATE);
    if (total_samples > max_samples)
        total_samples = max_samples;

    float phase = 0.0f;

    // Precompute constants
    float pitch_decay_factor = -p->pitch_decay * dt;
    float env_decay_factor = -p->env_curve / p->decay * dt;
    float freq_base = p->frequency * dt;

    int waveform = p->waveform_id;

    float amp = p->amplitude;
    float dc_offset = p->offset_dc;

    // Precompute caches
    static float envelope_cache[16384];
    static float pitch_cache[16384];

    // check if we need to predetermine caches
    bool use_env_cache = (env_decay_factor != 0.0f);
    bool use_pitch_cache = (pitch_decay_factor != 0.0f);
    bool use_dc_offset = (dc_offset != 0.0f);

    if (use_env_cache) {
        compute_envelope_cache(envelope_cache, total_samples, env_decay_factor);
    }

    if (use_pitch_cache) {
        compute_pitch_decay_cache(pitch_cache, total_samples, pitch_decay_factor);
    }

    static const float rand_scale = 2.0f / (float) RAND_MAX;

    // gen sample
    for (int i = 0; i < total_samples; i++) {
        // pitch decay
        float freq_mult = use_pitch_cache ? pitch_cache[i] : 1.0f;
        float phase_inc = freq_base * freq_mult;

        // wrap + accumulate phase
        phase += phase_inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // dc offset phase shift (only compute if needed)
        float phase_shifted = phase;
        if (use_dc_offset) {
            phase_shifted += dc_offset;
            if (phase_shifted >= 1.0f)
                phase_shifted -= 1.0f;
        }

        float val;
        switch (waveform) {
        case 0: // Sine wave
            val = fast_sin(phase_shifted);
            break;
        case 1: // Square wave
            val = (phase_shifted < 0.5f) ? 1.0f : -1.0f;
            break;
        case 2: // Triangle wave
            val = (phase_shifted < 0.5f) ? (4.0f * phase_shifted - 1.0f)
                                         : (3.0f - 4.0f * phase_shifted);
            break;
        case 3: // Sawtooth wave
            val = 2.0f * phase_shifted - 1.0f;
            break;
        case 4: // Noise
            val = (float) rand() * rand_scale - 1.0f;
            break;
        default:
            val = 0;
            break;
        }

        // Apply envelope
        float env = use_env_cache ? envelope_cache[i] : 1.0f;
        val = amp * env * val;

        // Clamp to range
        if (val > 1.0f)
            val = 1.0f;
        else if (val < -1.0f)
            val = -1.0f;

        // Convert to PWM
        pwm_buffer[i] = (uint16_t) ((val + 1.0f) * 127.5f);
    }

    // Fill with silence
    uint16_t silence = 255 / 2;
    for (int i = total_samples; i < max_samples; i++) {
        pwm_buffer[i] = silence;
    }

    return max_samples;
}
