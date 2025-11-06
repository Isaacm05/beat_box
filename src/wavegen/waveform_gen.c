#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100.0f
#define PI 3.14159265f

typedef struct {
    float frequency;
    float amplitude;
    float decay;
    int waveform_id; // 0=sine, 1=square, 2=triangle, 3=saw, 4=noise
    float offset_dc;
    float pitch_decay;
    float noise_mix;
    float env_curve;
    float comp_amount;
} WaveParams;

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
    float val = 0.0f;
    float freq, env;

    for (int i = 0; i < total_samples; i++) {
        float t = i * dt;
        freq = p->frequency * expf(-p->pitch_decay * t) *
               (1.0f + ((rand() / (float) RAND_MAX) - 0.5f) * 0.004f);
        phase += freq * dt;
        if (phase >= 1.0f)
            phase -= 1.0f;

        switch (p->waveform_id) {
        case 0:
            val = wave_sine(phase);
            break;
        case 1:
            val = wave_square(phase);
            break;
        case 2:
            val = wave_triangle(phase);
            break;
        case 3:
            val = wave_saw(phase);
            break;
        case 4:
            val = wave_noise();
            break;
        default:
            val = 0.0f;
            break;
        }

        // Blend with noise
        if (p->noise_mix > 0.0f) {
            float n = wave_noise();
            val = (1.0f - p->noise_mix) * val + p->noise_mix * n;
        }

        // Envelope
        env = expf(-p->env_curve * t / p->decay);
        val = p->amplitude * env * val + p->offset_dc;

        // Compression
        float comp = fmaxf(fminf(p->comp_amount, 1.0f), 0.0f);
        if (comp > 0.0f) {
            float threshold = 0.8f - 0.6f * comp;
            float ratio = (p->waveform_id == 0) ? (1.0f + 5.0f * comp) : (1.0f + 9.0f * comp);
            float makeup = (p->waveform_id == 0) ? (1.0f + 0.6f * comp) : (1.0f + 1.0f * comp);
            float abs_sig = fabsf(val);
            if (abs_sig > threshold) {
                float gain = (threshold + (abs_sig - threshold) / ratio) / (abs_sig + 1e-8f);
                val *= gain;
            }
            val *= makeup;
            if (comp > 0.8f) {
                float limit_level = 1.0f - (1.0f - threshold) * 0.5f;
                if (val > limit_level)
                    val = limit_level;
                if (val < -limit_level)
                    val = -limit_level;
            }
        }

        // Low-end recovery for sine tones
        if (p->waveform_id == 0)
            val *= 1.1f;

        // Clamp
        if (val > 1.0f)
            val = 1.0f;
        if (val < -1.0f)
            val = -1.0f;

        buffer[i] = val;
    }

    return total_samples;
}
