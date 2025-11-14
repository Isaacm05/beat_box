#include "waveform_gen.h"
#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100.0f
#define PI 3.14159265f

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
