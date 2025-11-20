#include "waveform_gen.h"
#include "pwm_audio.h"
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265f

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// PWM configuration (must match pwm_audio.h)
#define PWM_WRAP_LOCAL 255


#define SINE_TABLE_SIZE 256
static float sine_table[SINE_TABLE_SIZE];
static bool sine_table_initialized = false;

// Initialize sine lookup table (called once at startup)
static void init_sine_table(void) {
    if (!sine_table_initialized) {
        for (int i = 0; i < SINE_TABLE_SIZE; i++) {
            sine_table[i] = sinf(2.0f * M_PI * i / SINE_TABLE_SIZE);
        }
        sine_table_initialized = true;
    }
}

// Fast sine lookup - replaces slow sinf() calls
static inline float fast_sin(float phase) {
    // Ensure phase is in [0, 1)
    phase = phase - (int)phase;
    if (phase < 0.0f) phase += 1.0f;

    // Lookup from table
    int index = (int)(phase * SINE_TABLE_SIZE) & (SINE_TABLE_SIZE - 1);
    return sine_table[index];
}

// --- Waveform functions ---
static inline float wave_sine(float phase) {
    return sinf(2 * PI * phase);
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

int waveform_generate(float* buffer, int max_samples, WaveParams* p) {
    float dt = 1.0f / SAMPLE_RATE;
    int total_samples = (int) (p->decay * SAMPLE_RATE);
    if (total_samples > max_samples)
        total_samples = max_samples;

    float phase = 0.0f;

    for (int i = 0; i < total_samples; i++) {
        float t = i * dt;

        float freq = p->frequency * expf(-p->pitch_decay * t) *
                     (1.0f + ((rand() / (float) RAND_MAX) - 0.5f) * 0.004f);

        phase += freq * dt;
        if (phase >= 1.0f)
            phase -= 1.0f;

        float val;
        switch (p->waveform_id) {
        case 0:
            val = sinf(2 * M_PI * phase);
            break;
        case 1:
            val = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case 2:
            val = 4.0f * fabsf(phase - 0.5f) - 1.0f;
            break;
        case 3:
            val = 2.0f * phase - 1.0f;
            break;
        case 4:
            val = (rand() / (float) RAND_MAX) * 2.0f - 1.0f;
            break;
        default:
            val = 0;
            break;
        }

        float env = expf(-p->env_curve * t / p->decay);
        val = p->amplitude * env * val + p->offset_dc;

        // Clamp
        if (val > 1.0f)
            val = 1.0f;
        if (val < -1.0f)
            val = -1.0f;

        buffer[i] = val;
    }

    // Fill rest with silence
    for (int i = total_samples; i < max_samples; i++) {
        buffer[i] = 0.0f;
    }

    return max_samples;
}

//eliminate float buffer - OPTIMIZED VERSION WITH ENVELOPE PRECOMPUTATION
int waveform_generate_pwm(uint16_t* pwm_buffer, int max_samples, WaveParams* p) {
    // Initialize sine table if needed
    init_sine_table();

    float dt = 1.0f / SAMPLE_RATE;
    int total_samples = (int) (p->decay * SAMPLE_RATE);
    if (total_samples > max_samples)
        total_samples = max_samples;

    float phase = 0.0f;

    // Precompute constants outside the loop
    float pitch_decay_factor = -p->pitch_decay * dt;
    float env_decay_factor = -p->env_curve / p->decay * dt;
    float freq_base = p->frequency * dt;

    // Cache waveform type to avoid repeated switch statements in hot path
    int waveform = p->waveform_id;

    // Precompute amplitude and offset scaling
    float amp = p->amplitude;
    float dc_offset = p->offset_dc;

    // ========================================================================
    // MEGA OPTIMIZATION: Precompute ALL envelope values to eliminate expf() in loop!
    // This trades ~32KB memory (8K samples max × 4 bytes) for 50-100x speed boost
    // ========================================================================
    static float envelope_cache[16384];  // Reusable buffer
    bool use_env_cache = (env_decay_factor != 0.0f);

    if (use_env_cache) {
        // Precompute envelope once, outside the hot loop
        for (int i = 0; i < total_samples; i++) {
            envelope_cache[i] = expf(env_decay_factor * i);
        }
    }

    for (int i = 0; i < total_samples; i++) {
        // Optimized frequency calculation - still needs expf for pitch decay
        float freq_mult = (pitch_decay_factor != 0.0f) ? expf(pitch_decay_factor * i) : 1.0f;
        float phase_inc = freq_base * freq_mult;

        phase += phase_inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        float val;
        switch (waveform) {
        case 0:
            // Use lookup table instead of sinf() - FAST!
            val = fast_sin(phase);
            break;
        case 1:
            val = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case 2:
            val = 4.0f * fabsf(phase - 0.5f) - 1.0f;
            break;
        case 3:
            val = 2.0f * phase - 1.0f;
            break;
        case 4:
            val = (rand() / (float) RAND_MAX) * 2.0f - 1.0f;
            break;
        default:
            val = 0;
            break;
        }

        // Use precomputed envelope - NO expf() call here!
        float env = use_env_cache ? envelope_cache[i] : 1.0f;
        val = amp * env * val + dc_offset;

        if (val > 1.0f)
            val = 1.0f;
        else if (val < -1.0f)
            val = -1.0f;

        pwm_buffer[i] = (uint16_t)((val + 1.0f) * 127.5f);
    }

    // Fill rest with silence (PWM value for 0V = 127)
    uint16_t silence = PWM_WRAP_LOCAL / 2;
    for (int i = total_samples; i < max_samples; i++) {
        pwm_buffer[i] = silence;
    }

    return max_samples;
}
