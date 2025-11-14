#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#include <stdint.h>

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

int waveform_generate(float* buffer, int max_samples, WaveParams* p);

#endif